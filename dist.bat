git pull
pip3 install filelock pylzma
pushd bin\Windows\x64_QT5.12.3\Release
git init
git add *
git commit -m "Commit"
popd
pushd bin\Windows\x86_QT5.12.3\Release
git init
git add *
git commit -m "Commit"
popd
py -3 patch_gen.py bin\Windows\x64_QT5.12.3\Release dist OverlayProc_x64_QT5.12.3
py -3 patch_gen.py bin\Windows\x86_QT5.12.3\Release dist OverlayProc_x86_QT5.12.3