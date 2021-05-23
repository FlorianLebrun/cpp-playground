
const char* text_sample_same[] = {
   // Text before
   R"--(A
B
C
D)--",
// Text after
R"--(A
B
C
D)--"
};

const char* text_sample_fulldiff[] = {
   // Text before
   R"--(A
B
C
D)--",
// Text after
R"--(E
F
H
G)--"
};

const char* text_sample0[] = {
   // Text before
   R"--(A
B
C
D)--",
// Text after
R"--(A
B
C
C
D)--"
};

const char* text_sample1[] = {
   // Text before
   R"--(
void Chunk_copy(Chunk *src, size_t src_start, Chunk *dst, size_t dst_start, size_t n)
{
    if (!Chunk_bounds_check(src, src_start, n)) return;
    if (!Chunk_bounds_check(dst, dst_start, n)) return;

    memcpy(dst->data + dst_start, src->data + src_start, n);
}

int Chunk_bounds_check(Chunk *chunk, size_t start, size_t n)
{
    if (chunk == NULL) return 0;

    return start <= chunk->length && n <= chunk->length - start;
}
)--",
// Text after
R"--(
int Chunk_bounds_check(Chunk *chunk, size_t start, size_t n)
{
    if (chunk == NULL) return 0;

    return start <= chunk->length && n <= chunk->length - start;
}

void Chunk_copy(Chunk *src, size_t src_start, Chunk *dst, size_t dst_start, size_t n)
{
    if (!Chunk_bounds_check(src, src_start, n)) return;
    if (!Chunk_bounds_check(dst, dst_start, n)) return;

    memcpy(dst->data + dst_start, src->data + src_start, n);
}
)--"
};
