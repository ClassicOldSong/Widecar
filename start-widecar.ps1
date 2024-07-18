param(
    [string]$exePath = "C:\Program Files\Sunshine\sunshine.exe",
    [string]$exeParams = ".\config\sunshine_widecar.conf",
    [string]$workingDirectory = "C:\Program Files\Sunshine\"
)

# Path to PsExec
# You can download it from https://learn.microsoft.com/en-us/sysinternals/downloads/psexec
$psexecPath = "`"D:\Tools\pstools\PsExec.exe`""

# Build the command
$command = "powershell -Command `"start '$exePath' '$exeParams' -WorkingDirectory '$workingDirectory' -WindowStyle Hidden`""

# Get the first non-zero session ID using query user
$users = query user
$sessionId = $null

for ($i=1; $i -lt $users.Count; $i++) {
    $temp = [string]($users[$i] | Select-String -Pattern "\s\d+\s").Matches.Value.Trim()
    if ($temp -ne '0') {
        $sessionId = $temp
        break
    }
}

# Execute the command
Start-Process -FilePath $psexecPath -ArgumentList "-accepteula -i $sessionId -s $command" -WindowStyle Hidden
