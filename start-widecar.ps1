param (
    [string]$exePath = "C:\Program Files\Sunshine\sunshine.exe",
    [string]$workingDirectory = "C:\Program Files\Sunshine\",
    # Adjust with the actual config files you want to start.
    # e.g. if you have only one widecar config, leave only the first entry.
    [string[]]$exeParams = @(".\config\sunshine_widecar.conf", ".\config\sunshine_widecar_2.conf", ".\config\sunshine_widecar_3.conf")
)

# Path to PsExec
# You can download it from https://learn.microsoft.com/en-us/sysinternals/downloads/psexec
$psexecPath = "D:\Tools\pstools\PsExec.exe"

# Generate commands dynamically
$commands = $exeParams | ForEach-Object {
    "powershell -Command `"start '$exePath' '$_' -WorkingDirectory '$workingDirectory' -WindowStyle Hidden`""
}

# Get the first non-zero session ID
$users = query user
$sessionId = $users | Select-String -Pattern "\s\d+\s" | ForEach-Object {
    $temp = $_.Matches.Value.Trim()
    if ($temp -ne '0') { return $temp }
} | Select-Object -First 1

# Execute each command
foreach ($command in $commands) {
    Start-Process -FilePath $psexecPath -ArgumentList "-accepteula -i $sessionId -s $command" -WindowStyle Hidden
    Start-Sleep -Seconds 1  # Optional: Add a delay to avoid conflicts
}