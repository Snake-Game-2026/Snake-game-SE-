#!/bin/sh
# Builds every user-story file in features/ and runs its self-test (same steps as the CI workflow).
# Usage (from the repository root):  sh scripts/run_self_tests.sh
set -e
mkdir -p bin
for f in features/*.cpp; do
  n=$(basename "$f" .cpp)
  g++ -std=c++17 -Wall -Wextra "$f" -o "bin/$n" -lsfml-graphics -lsfml-window -lsfml-system
  echo "== $n"
  ./bin/"$n" --test
done
echo "All self-tests passed."
