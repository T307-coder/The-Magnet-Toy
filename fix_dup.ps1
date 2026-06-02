$file = "c:\Users\wangs\The-Powder-Toy\src\simulation\Simulation.cpp"
$lines = [System.Collections.Generic.List[string]](Get-Content $file)

# Find GetNeighbourhood definition
$gnStart = -1
$gnEnd = -1
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match 'SimulationImpl::GetNeighbourhood') {
        $gnStart = $i - 1  # include the return type line
    }
    if ($gnStart -gt 0 -and $i -gt $gnStart -and $lines[$i] -eq '}' -and $i+2 -lt $lines.Count) {
        if ($lines[$i+2] -match 'void SimulationImpl::UpdateParticles') {
            $gnEnd = $i
            break
        }
    }
}
Write-Output "GetNeighbourhood: $($gnStart+1) to $($gnEnd+1)"

# Remove the duplicate old 3D loop block
# Find the SECOND occurrence of "for (auto nz=-1; nz<2; nz++)" after the Z-axis comment
$dupStart = -1
$foundFirstNZ = $false
for ($i = $gnStart; $i -le $gnEnd; $i++) {
    if ($lines[$i] -match '^\t\tfor \(auto nz=-1') {
        if (-not $foundFirstNZ) { $foundFirstNZ = $true }
        else { $dupStart = $i; break }
    }
}

if ($dupStart -gt 0) {
    # Find end of this duplicate block: the closing } before the gravity check
    $dupEnd = $dupStart
    $braceDepth = 0
    for ($i = $dupStart; $i -le $gnEnd; $i++) {
        $braceDepth += ($lines[$i].ToCharArray() | Where-Object { $_ -eq '{' }).Count
        $braceDepth -= ($lines[$i].ToCharArray() | Where-Object { $_ -eq '}' }).Count
        if ($braceDepth -eq 0 -and $i -gt $dupStart) {
            $dupEnd = $i
            break
        }
    }
    Write-Output "Duplicate block: lines $($dupStart+1) to $($dupEnd+1)"
    for ($r = $dupEnd; $r -ge $dupStart; $r--) {
        $null = $lines.RemoveAt($r)
    }
    $lines | Set-Content $file -Encoding UTF8
    Write-Output "DUPLICATE REMOVED"
} else {
    Write-Output "No duplicate found"
}
