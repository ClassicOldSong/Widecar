param(
    [string]$exePath = "C:\Program Files\Sunshine\sunshine.exe",
    [string]$exeParams = ".\config\sunshine_widecar.conf",
    [string]$workingDirectory = "C:\Program Files\Sunshine\"
)

# Path to PsExec
# You can download it from https://learn.microsoft.com/en-us/sysinternals/downloads/psexec
$psexecPath = "`"D:\Tools\pstools\PsExec.exe`""

# Build the command
if ($exeParams) {
    $command = "powershell -Command `"start '$exePath' '$exeParams' -WorkingDirectory '$workingDirectory' -WindowStyle Hidden`""
} else {
    $command = "powershell -Command `"start '$exePath' -WorkingDirectory '$workingDirectory' -WindowStyle Hidden`""
}

# Execute the command
Start-Process -FilePath $psexecPath -ArgumentList "-accepteula -i 1 -s $command" -WindowStyle Hidden
