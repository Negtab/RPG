<?php
// Принимает баг-репорт с формы сайта и пересылает его в Telegram.
// Токен бота и chat_id лежат в bugreport.config.php (в git не попадает, см. bugreport.config.example.php)

header('Content-Type: application/json; charset=utf-8');

function reply(int $code, array $body): void
{
    http_response_code($code);
    echo json_encode($body, JSON_UNESCAPED_UNICODE);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') reply(405, ['ok' => false, 'error' => 'Только POST']);

$configFile = __DIR__ . '/bugreport.config.php';
if (!is_file($configFile)) reply(500, ['ok' => false, 'error' => 'Сервер не настроен']);
$config = require $configFile;
$token = $config['bot_token'] ?? '';
$chatId = $config['chat_id'] ?? '';
if ($token === '' || $chatId === '') reply(500, ['ok' => false, 'error' => 'Сервер не настроен']);

// ── Защита от спама ──
// Скрытое поле: люди его не видят и не заполняют, боты заполняют
if (!empty($_POST['website'])) reply(200, ['ok' => true]);
// Форму отправили быстрее чем за 3 секунды после открытия — это бот
$openedAt = (int)($_POST['opened_at'] ?? 0);
if ($openedAt > 0 && time() * 1000 - $openedAt < 3000) reply(429, ['ok' => false, 'error' => 'Слишком быстро, попробуйте ещё раз']);

// Не больше 5 репортов с одного IP за час
$limitDir = __DIR__ . '/.bugreport-limits';
if (!is_dir($limitDir)) @mkdir($limitDir, 0700);
$ip = $_SERVER['REMOTE_ADDR'] ?? 'unknown';
$limitFile = $limitDir . '/' . hash('sha256', $ip);
$hits = is_file($limitFile) ? array_filter(explode("\n", (string)file_get_contents($limitFile))) : [];
$hits = array_values(array_filter($hits, fn($t) => (int)$t > time() - 3600));
if (count($hits) >= 5) reply(429, ['ok' => false, 'error' => 'Слишком много репортов за час. Попробуйте позже']);

// ── Поля формы ──
function field(string $name, int $max): string
{
    $value = trim((string)($_POST[$name] ?? ''));
    $value = preg_replace('/\r\n?/', "\n", $value);
    return mb_substr($value, 0, $max);
}

$title    = field('title', 120);
$category = field('category', 40);
$details  = field('details', 2000);
$steps    = field('steps', 1500);
$version  = field('version', 30);
$os       = field('os', 60);
$contact  = field('contact', 100);

if (mb_strlen($title) < 3)   reply(400, ['ok' => false, 'error' => 'Опишите проблему в заголовке']);
if (mb_strlen($details) < 10) reply(400, ['ok' => false, 'error' => 'Расскажите подробнее, что случилось']);

// ── Скриншот ──
$photo = null;
if (!empty($_FILES['screenshot']) && $_FILES['screenshot']['error'] !== UPLOAD_ERR_NO_FILE) {
    $f = $_FILES['screenshot'];
    if ($f['error'] !== UPLOAD_ERR_OK) reply(400, ['ok' => false, 'error' => 'Не удалось загрузить скриншот']);
    if ($f['size'] > 8 * 1024 * 1024) reply(400, ['ok' => false, 'error' => 'Скриншот больше 8 МБ']);
    $info = @getimagesize($f['tmp_name']);
    $types = [IMAGETYPE_PNG => 'png', IMAGETYPE_JPEG => 'jpg', IMAGETYPE_WEBP => 'webp', IMAGETYPE_GIF => 'gif'];
    if (!$info || !isset($types[$info[2]])) reply(400, ['ok' => false, 'error' => 'Скриншот должен быть PNG, JPG, WEBP или GIF']);
    $photo = new CURLFile($f['tmp_name'], $info['mime'], 'screenshot.' . $types[$info[2]]);
}

// ── Сообщение ──
$h = fn(string $s) => htmlspecialchars($s, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
$lines = ['🐞 <b>' . $h($title) . '</b>'];
$meta = array_filter([
    $category !== '' ? '📂 ' . $h($category) : '',
    $version !== '' ? '🎮 ' . $h($version) : '',
    $os !== '' ? '💻 ' . $h($os) : '',
]);
if ($meta) $lines[] = implode('  ·  ', $meta);
$lines[] = '';
$lines[] = '<b>Что случилось:</b>';
$lines[] = $h($details);
if ($steps !== '') {
    $lines[] = '';
    $lines[] = '<b>Как повторить:</b>';
    $lines[] = $h($steps);
}
if ($contact !== '') {
    $lines[] = '';
    $lines[] = '✉️ ' . $h($contact);
}
$lines[] = '';
$lines[] = '<i>' . date('d.m.Y H:i') . ' · ' . $h($ip) . '</i>';
$text = implode("\n", $lines);

function telegram(string $token, string $method, array $params): array
{
    $ch = curl_init("https://api.telegram.org/bot{$token}/{$method}");
    curl_setopt_array($ch, [
        CURLOPT_POST => true,
        CURLOPT_POSTFIELDS => $params,
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT => 20,
    ]);
    $raw = curl_exec($ch);   // curl_close не нужен с PHP 8.0, а в 8.5 он помечен устаревшим
    $data = json_decode((string)$raw, true);
    return is_array($data) ? $data : ['ok' => false];
}

$sent = telegram($token, 'sendMessage', [
    'chat_id' => $chatId,
    'text' => $text,
    'parse_mode' => 'HTML',
    'disable_web_page_preview' => 'true',
]);
if (empty($sent['ok'])) reply(502, ['ok' => false, 'error' => 'Не удалось отправить репорт. Попробуйте позже']);

// Скриншот — ответом на сообщение, чтобы они шли парой
if ($photo) {
    $params = ['chat_id' => $chatId, 'reply_to_message_id' => $sent['result']['message_id']];
    $result = telegram($token, 'sendPhoto', $params + ['photo' => $photo]);
    if (empty($result['ok']))   // слишком большое или странное для фото — отправим файлом
        telegram($token, 'sendDocument', $params + ['document' => $photo]);
}

$hits[] = (string)time();
@file_put_contents($limitFile, implode("\n", $hits));

reply(200, ['ok' => true]);
