# =============================================================================
# Simulated AI Agent - create a box with looping translate animation via MCP
# Simulates an LLM agent's "think -> call tool -> observe -> next step" loop
# Usage: .\agent.ps1 [-Port 28100]
# =============================================================================

param(
    [int]$Port = 28100
)

$ErrorActionPreference = 'Stop'
$Endpoint = "http://127.0.0.1:$Port/mcp"

function Write-Thought {
    param([string]$Text)
    Write-Host ""
    Write-Host "=== [THINK] ===" -ForegroundColor Cyan
    Write-Host $Text -ForegroundColor Cyan
}

function Write-Step {
    param([string]$Text)
    Write-Host ""
    Write-Host "--- [STEP] $Text" -ForegroundColor Yellow
}

function Invoke-McpTool {
    param(
        [string]$Name,
        [hashtable]$Arguments,
        [int]$Id
    )
    $body = @{
        jsonrpc = "2.0"
        id      = $Id
        method  = "tools/call"
        params  = @{
            name      = $Name
            arguments = $Arguments
        }
    } | ConvertTo-Json -Depth 12 -Compress

    Write-Host "  [TOOL_CALL] $Name " -NoNewline -ForegroundColor Green
    Write-Host ($Arguments | ConvertTo-Json -Depth 8 -Compress) -ForegroundColor Gray

    $resp = Invoke-RestMethod -Uri $Endpoint -Method Post -ContentType 'application/json' -Body $body
    $result = $resp.result
    $text   = $result.content[0].text

    if ($result.isError) {
        Write-Host "  [TOOL_RESULT] ERROR: $text" -ForegroundColor Red
        throw "tool $Name failed: $text"
    }
    Write-Host "  [TOOL_RESULT] OK: $text" -ForegroundColor Green
    return ($text | ConvertFrom-Json)
}

Write-Host "============================================================" -ForegroundColor Magenta
Write-Host " Simulated AI Agent via MCP" -ForegroundColor Magenta
Write-Host " Goal: create a box with looping translate animation" -ForegroundColor Magenta
Write-Host "============================================================" -ForegroundColor Magenta

# Step 1
Write-Thought "The user wants me to create a box with animation. Before acting, I should inspect the current scene to avoid conflicts."
Write-Step "1. Query current scene"
$scene = Invoke-McpTool -Name "scene_info" -Arguments @{} -Id 1
Write-Host "  -> current node count: $($scene.count)" -ForegroundColor Gray

# Step 2
Write-Thought "Scene only has a ground grid. I need to create a node to hold the box mesh, name it 'box'."
Write-Step "2. Create node 'box'"
$node = Invoke-McpTool -Name "node_create" -Arguments @{ name = "box" } -Id 2
Write-Host "  -> node '$($node.name)' created" -ForegroundColor Gray

# Step 3
Write-Thought "Node is ready. Now create a box mesh via geo_create with type=box and size 1x1x1."
Write-Step "3. Create box geometry"
$geo = Invoke-McpTool -Name "geo_create" -Arguments @{
    type   = "box"
    params = @{ size = @{ x = 1; y = 1; z = 1 } }
} -Id 3
Write-Host "  -> meshId=$($geo.meshId), vertices=$($geo.vertexCount)" -ForegroundColor Gray

# Step 4
Write-Thought "Mesh created with meshId $($geo.meshId). Attach it to the 'box' node."
Write-Step "4. Attach mesh to node"
Invoke-McpTool -Name "node_set_mesh" -Arguments @{
    node   = "box"
    meshId = $geo.meshId
} -Id 4 | Out-Null

# Step 5
Write-Thought "Set the box at x=-3 so the animation moving to x=3 is clearly visible."
Write-Step "5. Set initial transform"
Invoke-McpTool -Name "node_set_transform" -Arguments @{
    node        = "box"
    translation = @{ x = -3; y = 0.5; z = 0 }
} -Id 5 | Out-Null

# Step 6
Write-Thought "Now create the translate animation: two keyframes, t=0 at (-3,0.5,0) and t=2s at (3,0.5,0), looping."
Write-Step "6. Create looping translate animation"
$anim = Invoke-McpTool -Name "anim_translate" -Arguments @{
    node      = "box"
    keyframes = @(
        @{ t = 0; x = -3; y = 0.5; z = 0 }
        @{ t = 2; x =  3; y = 0.5; z = 0 }
    )
    loop      = $true
} -Id 6
Write-Host "  -> animation status: $($anim.note)" -ForegroundColor Gray

# Step 7
Write-Thought "Animation created. Verify the final scene state and node transform."
Write-Step "7. Verify scene state"
$final = Invoke-McpTool -Name "scene_info" -Arguments @{} -Id 7
Write-Host "  -> final node count: $($final.count)" -ForegroundColor Gray
foreach ($n in $final.nodes) {
    $hasMesh = if ($n.mesh) { "mesh=yes" } else { "mesh=no" }
    Write-Host "     - $($n.name) [$hasMesh, children=$($n.childCount)]" -ForegroundColor Gray
}

$xform = Invoke-McpTool -Name "node_get_transform" -Arguments @{ node = "box" } -Id 8
Write-Host "  -> box localTranslation = ($($xform.translation.x), $($xform.translation.y), $($xform.translation.z))" -ForegroundColor Gray

Write-Host ""
Write-Host "============================================================" -ForegroundColor Magenta
Write-Host " Agent task complete" -ForegroundColor Green
Write-Host " box created with mesh, looping translate animation is playing" -ForegroundColor Green
Write-Host " Observe the render window: box should move along X axis" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Magenta
