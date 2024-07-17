
cat testfile.wav | socat - TCP:192.168.10.71:8000 | ffmpeg -f s16le -ar 16000 -ac 2 -i - -ac 1 -f s16le - | ffplay -nodisp -f s16le -ar 16000 -
