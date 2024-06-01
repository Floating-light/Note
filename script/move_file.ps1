# 定义源文件夹路径
$sourceFolderPath = "E:\Workspace\Note\"

# 获取所有以 .md 结尾的文件
$files = Get-ChildItem -Path $sourceFolderPath -Filter "*.md"

foreach ($file in $files) {
    # 获取文件名（不含扩展名）
    $fileNameWithoutExtension = [System.IO.Path]::GetFileNameWithoutExtension($file.Name)

    # 以 "." 分割文件名
    $fileNameParts = $fileNameWithoutExtension.Split(".")

    # 取第一个部分作为文件夹名
    $folderName = $fileNameParts[0]

    # 如果文件名分割后有两个或更多部分，则去掉第一个部分组织新的文件名
    if ($fileNameParts.Length -ge 2) {
        $newFileName = ($fileNameParts[1..($fileNameParts.Length - 1)] -join ".") + ".md"
    } else {
        $newFileName = $file.Name
    }

    # 创建目标文件夹路径
    $destinationFolderPath = Join-Path -Path $sourceFolderPath -ChildPath $folderName

    # 如果目标文件夹不存在，则创建它
    if (-not (Test-Path -Path $destinationFolderPath)) {
        New-Item -Path $destinationFolderPath -ItemType Directory | Out-Null
    }

    # 构建目标文件路径
    $destinationFilePath = Join-Path -Path $destinationFolderPath -ChildPath $newFileName

    # 移动文件并重命名
    Move-Item -Path $file.FullName -Destination $destinationFilePath
}

Write-Output "文件移动完成。"