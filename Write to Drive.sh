#!/bin/zsh

if [ -d /Volumes/Budget ]; then
  cp dist/Windows/BudgetInstaller.exe /Volumes/Budget/BudgetInstaller.exe || {
    echo "Error while copying Windows installer to drive"
    exit 1
  }
  echo "Write successful!"
else
  echo "Could not find drive"
fi
