"""Заливает папку website/ на хостинг по FTPS: только новые и изменённые файлы.

Запуск из корня проекта:
    python tools/deploy_site.py            # залить изменения
    python tools/deploy_site.py --dry-run  # только показать, что изменится
    python tools/deploy_site.py --prune    # ещё и удалить с сервера файлы, которых нет локально

Доступ лежит в tools/deploy.ini (в git не попадает, образец — tools/deploy.example.ini).
Хэши залитых файлов — в tools/.deploy-state.json: так скрипт знает, что уже на сервере.
Удаляет он только то, что заливал сам, и только с --prune. Страницы заливаются последними,
чтобы кнопка «Скачать» не вела на файл, который ещё не загружен.
"""
import argparse
import configparser
import ftplib
import hashlib
import io
import json
import posixpath
import secrets
import ssl
import sys
import urllib.request
from pathlib import Path
from urllib.parse import quote

ROOT = Path(__file__).resolve().parent.parent
SITE = ROOT / "website"
CONFIG = ROOT / "tools" / "deploy.ini"
STATE = ROOT / "tools" / ".deploy-state.json"
SKIP_NAMES = {"bugreport.config.example.php"}
SKIP_DIRS = {".bugreport-limits"}
LAST_SUFFIXES = {".html", ".php"}                 # после картинок и установщика
RETRIES = 8
CHUNK = 16 << 20                                  # меньше 3 минут при 100 КБ/с

JOIN_PHP = """<?php
// Одноразовый: собирает части в файл, сверяет SHA-256 и удаляет себя
if (($_GET['k'] ?? '') !== '@KEY@') { http_response_code(404); exit; }
header('Content-Type: text/plain');
$parts = [@PARTS@];
$out = fopen(__DIR__ . '/@NAME@.joining', 'wb');
foreach ($parts as $p) {
    $in = fopen(__DIR__ . '/' . $p, 'rb');
    stream_copy_to_stream($in, $out);
    fclose($in);
}
fclose($out);
if (hash_file('sha256', __DIR__ . '/@NAME@.joining') === '@HASH@') {
    rename(__DIR__ . '/@NAME@.joining', __DIR__ . '/@NAME@');
    foreach ($parts as $p) unlink(__DIR__ . '/' . $p);
    echo 'ok';
} else {
    unlink(__DIR__ . '/@NAME@.joining');
    echo 'hash mismatch';
}
unlink(__FILE__);
"""


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def local_files() -> dict[str, str]:
    files = {}
    for path in SITE.rglob("*"):
        rel = path.relative_to(SITE)
        if path.is_file() and path.name not in SKIP_NAMES and not SKIP_DIRS & set(rel.parts):
            files[rel.as_posix()] = sha256(path)
    return files


def load_state() -> dict[str, str]:
    return json.loads(STATE.read_text(encoding="utf8")) if STATE.is_file() else {}


def save_state(state: dict[str, str]) -> None:
    STATE.write_text(json.dumps(state, indent=1, sort_keys=True), encoding="utf8")


def connect(cfg: configparser.SectionProxy) -> ftplib.FTP:
    host, port = cfg["host"], cfg.getint("port", 21)
    if cfg.getboolean("plain_ftp", False):            # только для теста на своей машине
        ftp = ftplib.FTP()
        ftp.connect(host, port, timeout=60)
        ftp.login(cfg["user"], cfg["password"])
    else:
        # У FTP хостинга самоподписанный сертификат: вместо центра сертификации сверяем его отпечаток
        fingerprint = cfg.get("tls_fingerprint", "").replace(":", "").lower()
        context = ssl.create_default_context()
        if fingerprint:
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
        ftp = ftplib.FTP_TLS(context=context)
        ftp.connect(host, port, timeout=60)
        ftp.auth()                                    # TLS включается до отправки пароля
        if fingerprint:
            actual = hashlib.sha256(ftp.sock.getpeercert(binary_form=True)).hexdigest()
            if actual != fingerprint:
                ftp.close()
                raise SystemExit(f"Сертификат FTP-сервера изменился: {actual}. Пароль не отправлен")
        ftp.login(cfg["user"], cfg["password"])
        ftp.prot_p()
    ftp.set_pasv(True)
    ftp.cwd(cfg["remote_dir"])
    return ftp


def ensure_dir(ftp: ftplib.FTP, remote_dir: str, made: set[str]) -> None:
    path = ""
    for part in remote_dir.split("/"):
        path = posixpath.join(path, part)
        if path in made:
            continue
        try:
            ftp.mkd(path)
        except ftplib.error_perm:                     # папка уже есть
            pass
        made.add(path)


def with_retry(ftp: ftplib.FTP | None, cfg: configparser.SectionProxy, action) -> ftplib.FTP:
    """Выполняет action(ftp), при обрыве подключается заново. Возвращает живое соединение."""
    for attempt in range(RETRIES, 0, -1):
        try:
            ftp = ftp or connect(cfg)
            action(ftp)
            return ftp
        except (OSError, EOFError, ftplib.Error) as e:
            if attempt == 1:
                raise
            print(f"обрыв ({e}), подключаюсь заново")
            if ftp:
                ftp.close()
            ftp = None
    raise AssertionError


def store(ftp: ftplib.FTP, rel: str, data: bytes | None = None) -> None:
    temp = rel + ".uploading"
    if data is None:
        with (SITE / rel).open("rb") as f:
            ftp.storbinary(f"STOR {temp}", f)
    else:
        ftp.storbinary(f"STOR {temp}", io.BytesIO(data))
    try:
        ftp.delete(rel)                               # не все серверы заменяют файл при RNTO
    except ftplib.error_perm:
        pass
    ftp.rename(temp, rel)


def upload_big(ftp: ftplib.FTP, cfg: configparser.SectionProxy, rel: str) -> ftplib.FTP:
    """Связь с хостингом около 100 КБ/с, передачи дольше ~9 минут рвутся, докачки на сервере нет.
    Поэтому части заливаются по FTP, а собирает их на сервере одноразовый PHP-скрипт."""
    path = SITE / rel
    folder, name = posixpath.split(rel)
    key, script = secrets.token_hex(16), f"join-{secrets.token_hex(8)}.php"
    parts = []
    with path.open("rb") as f:
        while chunk := f.read(CHUNK):
            part = f"{name}.part{len(parts):03}"
            parts.append(part)
            print(f"  часть {len(parts)}: {len(chunk) >> 20} МБ")
            ftp = with_retry(ftp, cfg, lambda c, r=posixpath.join(folder, part), d=chunk: store(c, r, d))

    php = JOIN_PHP.replace("@KEY@", key).replace("@NAME@", name).replace("@HASH@", sha256(path))
    php = php.replace("@PARTS@", ", ".join(f"'{p}'" for p in parts))
    remote_script = posixpath.join(folder, script)
    ftp = with_retry(ftp, cfg, lambda c: store(c, remote_script, php.encode("utf8")))
    url = f"{cfg['site_url'].rstrip('/')}/{quote(remote_script)}?k={key}"
    try:
        with urllib.request.urlopen(url, timeout=120) as response:
            answer = response.read().decode("utf8", "replace").strip()
    finally:
        def cleanup(c: ftplib.FTP) -> None:
            try:
                c.delete(remote_script)               # обычно скрипт уже удалил себя сам
            except ftplib.error_perm:
                pass
        ftp = with_retry(ftp, cfg, cleanup)
    if answer != "ok":
        raise SystemExit(f"Сервер не собрал {rel}: {answer[:200]}")
    return ftp


def main() -> int:
    parser = argparse.ArgumentParser(description="Заливает website/ на хостинг")
    parser.add_argument("--dry-run", action="store_true", help="только показать изменения")
    parser.add_argument("--prune", action="store_true", help="удалить с сервера файлы, которых нет локально")
    parser.add_argument("--force", action="store_true", help="залить всё заново")
    args = parser.parse_args()

    sys.stdout.reconfigure(encoding="utf-8")
    files, state = local_files(), load_state()
    changed = [rel for rel, digest in files.items() if args.force or state.get(rel) != digest]
    changed.sort(key=lambda rel: (Path(rel).suffix in LAST_SUFFIXES, rel))
    removed = sorted(set(state) - set(files))

    for rel in changed:
        print("залить ", rel)
    for rel in removed:
        print("удалить" if args.prune else "лишний ", rel)
    if not changed and not (removed and args.prune):
        print("Всё уже на сервере")
        return 0
    if args.dry_run:
        print(f"Проверка: залить {len(changed)}, лишних на сервере {len(removed)}")
        return 0

    if not CONFIG.is_file():
        print(f"Нет {CONFIG}: скопируйте deploy.example.ini в deploy.ini и заполните", file=sys.stderr)
        return 1
    parser_ini = configparser.ConfigParser(interpolation=None)
    parser_ini.read(CONFIG, encoding="utf8")
    cfg = parser_ini["ftp"]
    ftp = connect(cfg)
    made: set[str] = set()
    try:
        for i, rel in enumerate(changed, 1):
            parent = posixpath.dirname(rel)
            if parent:
                ftp = with_retry(ftp, cfg, lambda c: ensure_dir(c, parent, made))
            if (SITE / rel).stat().st_size > CHUNK:
                ftp = upload_big(ftp, cfg, rel)
            else:
                ftp = with_retry(ftp, cfg, lambda c: store(c, rel))
            state[rel] = files[rel]
            save_state(state)                         # обрыв связи не заставит заливать всё заново
            print(f"[{i}/{len(changed)}] {rel}")
        if args.prune:
            for rel in removed:
                try:
                    ftp.delete(rel)
                except ftplib.error_perm as e:
                    print(f"не удалось удалить {rel}: {e}", file=sys.stderr)
                    continue
                del state[rel]
                save_state(state)
    finally:
        try:
            ftp.quit()
        except (OSError, EOFError, ftplib.Error):
            ftp.close()
    print("Готово")
    return 0


if __name__ == "__main__":
    sys.exit(main())
