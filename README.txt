This program restores a grayscale image from “infused” ASCII lines and emits a 
valid PGM P5 file. The input is not a PGM; it’s plain text where each line 
contains decimal digit runs (the pixel values) interleaved with non-digits 
(the infusion pattern). We read the file once, group lines by their identical 
non-digit pattern, pick the group with the most lines as the original image, and 
output that group as a binary P5 image. The code uses Hanson’s CII libraries 
(except.h, atom.h, table.h, seq.h, mem.h) and a spec-compliant readaline with a 
1000-byte cap for partial credit.

I/O and format. If a filename is provided (./restoration file.txt), we read that 
file; otherwise we read from stdin. On success the program writes only a P5 
image to stdout: an ASCII header P5\n<width> <height>\n255\n followed by raw 
bytes (one byte per pixel) in row-major order. There is no extra printing—any 
chatter would corrupt the binary image. 

To use the tool: ./restoration infused.txt > out.pgm or 
cat infused.txt | ./restoration > out.pgm. View with an image viewer that 
supports PGM, or inspect with a hex viewer to see the binary raster after the 
header.

Algorithm (single pass). For each input line we build the pattern key by copying 
all non-digit bytes up to the end of line (handling both \n and \r\n), then 
intern that text with Atom_string to get a canonical const char* key. We also 
compact the line’s digit runs ([0–9]+) into bytes in place: parse each run as 
base-10, require 0..255, and store that single byte into the front of the 
buffer. The count of bytes written is the row width. We ignore lines that yield 
zero pixels. We then insert the row buffer into a Table that maps Atom → Bucket. 
A Bucket is a struct { size_t width; Seq_T rows; }, where rows is a sequence of 
char* row buffers. The first row seen for a pattern sets its width; all later 
rows for that pattern must match it. While ingesting, we track the pattern 
whose bucket has the largest Seq_length—that’s the image we will output. 
After EOF we write the P5 header using that bucket’s (width, height) and 
fwrite each stored row (exactly width bytes each).

Errors and edge cases. We use checked runtime errors (Hanson RAISE) for: bad 
arguments (more than one filename), open failures, read errors, any parsed 
pixel >255, inconsistent widths within a pattern, and write failures (short 
fwrite or a final fflush error). readaline is spec-compliant: it returns 0 and 
sets *datapp=NULL at EOF; if a line exceeds the 1000-byte cap without a newline 
it prints exactly readaline: input line too long\n to stderr and exits with code 
4 (as required). Parsing is isdigit-safe (we cast to unsigned char), and CRLF is 
handled by stopping on '\n' or '\r' when building the pattern and compacting 
digits.

Memory and performance. Each input line is allocated by readaline, compacted in 
place, then optionally RESIZEd to the exact row width and stored in a bucket’s 
sequence. After output we free: every stored row buffer, every Seq_T, and every 
Bucket, then the Table. Atoms are not freed (they are process-lifetime by 
design); Valgrind will report them as “still reachable,” which is acceptable as 
long as you have 0 bytes definitely/indirectly lost. The program is linear in 
the size of the input; memory usage is proportional to the number of rows 
retained in the winning bucket (plus smaller buckets during ingest). We cannot 
stream rows as we read them because the P5 header requires the final height; we 
buffer rows, then write header + raster.

Build and run. The provided Makefile links against the course CII libs. 
Run make to build. Usage examples:
./restoration infused.txt > out.pgm
cat infused.txt | ./restoration > out.pgm
Validate with a viewer or file out.pgm. For leaks: valgrind --leak-check=full 
--show-leak-kinds=all ./restoration < infused.txt > /dev/null and confirm 0 
definite/indirect; “still reachable” from Atoms is intentional.