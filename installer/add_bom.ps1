$path = Join-Path $PSScriptRoot 'CompactVLMQt.nsi'
$content = Get-Content $path -Raw -Encoding UTF8
$enc = New-Object System.Text.UTF8Encoding($true)
[System.IO.File]::WriteAllText($path, $content, $enc)
Write-Host "UTF-8 BOM applied: $path"
