for f in /comp/40/bin/images/corruption/*; do
    base=$(basename "$f")
    ./restoration "$f" > "testresults/${base}.out"
done