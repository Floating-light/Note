## 各种仓库损坏修复
https://git.seveas.net/repairing-and-recovering-broken-git-repositories.html

## 查看提交记录
1. 查看特定文件的提交记录
git log -- <file-path>
例如，如果你想查看文件 example.txt 的提交记录：
git log -- example.txt
1. 查看特定文件夹的提交记录
git log -- <folder-path>/
例如，如果你想查看文件夹 src 下任何文件的提交记录：
git log -- src/
1. 更详细的输出格式
如果你想要更详细的信息，比如每个提交的差异，你可以使用 -p 选项：
git log -p -- <file-or-folder-path>
例如查看 example.txt 中每个提交的差异：
git log -p -- example.txt
1. 简洁的输出格式
如果你希望输出更加简洁，可以使用 --oneline 选项：
git log --oneline -- <file-or-folder-path>
例如简要查看 src 文件夹的提交记录：
git log --oneline -- src/
1. 显示附加信息
例如显示每个提交的作者、日期等附加信息，你可以使用以下选项组合：
git log --pretty=format:"%h - %an, %ar : %s" -- <file-or-folder-path>
该命令的输出格式包括：
%h: 提交哈希值的缩写
%an: 作者名称
%ar: 相对的提交时间
%s: 提交消息
例如查看 docs/ 文件夹的提交记录：
git log --pretty=format:"%h - %an, %ar : %s" -- docs/
1. 输出包含特定关键字的提交信息
你可以使用 -G 选项来筛选包含特定关键字的提交信息。例如，查找包含 "fix bug" 的提交：
git log -G"fix bug" -- <file-or-folder-path>
例如查看文件 example.txt 中包含 "fix bug" 的提交：
git log -G"fix bug" -- example.txt
通过这些命令，你可以根据需求查看指定文件或文件夹的提交历史。根据需要，你可以调整以上命令中的选项来获取更加适合的信息格式。

## fork point, rebase
本质是对比两个分支的历史提交线，对所有的commitid按顺序做相减，把另一分支不存在的，都cherry-pick过去，再移动HEAD到最新Commitid。

- git merge-base --fork-point origin/TestBranch TestBranch
  - 查看公共起点，可以试试不加--fork-point

远端如果强推掉了一些历史提交记录，本地pull下来时，如果加了--fork-point,会参考本地reflog，如果一些commitid之前存在，但是现在不在了，git也会跳过它们的pick。

git pull --rebase 默认加了这个--fork-point

如果是手动rebase，如果不想把别人推掉的提交pick回来，需要自己加上--fork-point

git rebase --fork-point 

git rebase --onto <newbase> <upstream>

https://stackoverflow.com/questions/42536989/git-pull-rebase-lost-commits-after-coworkers-git-push-force

https://stackoverflow.com/questions/42486141/git-rebase-commit-select-in-fork-point-mode

# git Submodule
git submodule add https://github.com/example/mysubmodule.git libs/mysubmodule
git submodule init
git submodule update
git submodule update --init
git submodule update --init --recursive 
