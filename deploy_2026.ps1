$source = "C:\Users\user\Desktop\cpp\C4D_MeshCheck\sdk_2026\build\bin\Release\plugins\C4D_MeshCheck"
$dest   = "\\vmware-host\Shared Folders\plugins\C4D_MeshCheck"

Write-Host "`n[DEPLOYMENT] Deploying C4D_MeshCheck to $dest..."

if (-not (Test-Path $dest)) {
    New-Item -ItemType Directory -Path $dest -Force | Out-Null
}

$xdl = Join-Path $dest "C4D_MeshCheck.xdl64"
if (Test-Path -LiteralPath $xdl) {
    $tempOld = Join-Path $dest ("C4D_MeshCheck.xdl64." + [System.Guid]::NewGuid().ToString().Substring(0,8) + ".old")
    try {
        Rename-Item -LiteralPath $xdl -NewName (Split-Path $tempOld -Leaf) -Force -ErrorAction Stop
        Write-Host "[NOTE] Renamed existing binary to $tempOld"
    } catch {
        try {
            Remove-Item -LiteralPath $xdl -Force -ErrorAction Stop
        } catch {
            Write-Host "[WARN] Could not rename or delete existing binary: $_"
        }
    }
}

Copy-Item -Path "$source\*" -Destination $dest -Recurse -Force

Get-ChildItem -LiteralPath $dest -Filter "*.old" -File | ForEach-Object {
    try { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue } catch {}
}

Write-Host "[SUCCESS] Plugin successfully deployed!`n"
