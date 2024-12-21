# I have this saved at C:\Users\{username}\OneDrive\Documents\WindowsPowerShell

function rl { # Reload powershell
    Write-Output "Restarting powershell..."
    & powershell -NoExit -Command "Set-Location -Path $(Get-Location)"
    exit
}

function rb {
    # Stop any running instances of "main.exe" without showing errors if not found
    try {
        Stop-Process -Name "main" -Force -ErrorAction Stop
    } catch {
        # No need to take action if the process is not running
    }

    # Call the build function
    b
    # Check if the build was successful
    if ($LASTEXITCODE -eq 0) {
        # Only run if build was successful
        run
    } else {
        Write-Host "Build failed. Not running the executable." -ForegroundColor Red
    }
}

function run {
    # Get the path to the executable
    $basePath = Get-Location
    $exePath = Join-Path -Path $basePath -ChildPath "build\main.exe"
    Write-Host "The executable path is: $exePath"

    # Ensure the file exists before running
    if (Test-Path $exePath) {
        Start-Process $exePath
    } else {
        Write-Host "Executable not found: $exePath" -ForegroundColor Red
    }
}

function b {
    # Get the path to the batch file
    $basePath = Get-Location
    $batPath = Join-Path -Path $basePath -ChildPath "build-windows.bat"

    # Ensure the batch file exists before running
    if (Test-Path $batPath) {
        # Run the batch file and wait for it to complete
        $process = Start-Process -FilePath $batPath -Wait -NoNewWindow -PassThru
        $global:LASTEXITCODE = $process.ExitCode # Explicitly set the exit code
        Write-Host "Build completed with exit code: $global:LASTEXITCODE"
    } else {
        Write-Host "Build script not found: $batPath" -ForegroundColor Red
        $global:LASTEXITCODE = 1 # Set an error code for missing file
    }
}

function hm {
    Set-Location W:\handmade
}

function gs {
    git add .
    git commit -m "Save"
}

function cim {
    param (
        [string]$message
    )

    if (-not $message) {
        Write-Output "Please provide a commit message."
        return
    }

    git add .
    git commit -m "$message"
}