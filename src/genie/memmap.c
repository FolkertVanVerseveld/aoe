#include "memmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILESZ 16
#define FUNCSZ 32
#define DESCSZ 64

#define CANSZ sizeof(size_t)

static size_t can_mask = 0x25a6fd7c;

struct blkhdr {
	char file[FILESZ];
	char func[FUNCSZ];
	char desc[DESCSZ];
	size_t lineno, size;
	size_t canary;
};

static struct vheap {
	// malloc determines order of blocks
	// all addresses are unique
	struct blkhdr **a;
	unsigned n, cap;
} heap = {.a = NULL};

#define parent(x) (((x)-1)/2)
#define right(x) (2*((x)+1))
#define left(x) (right(x)-1)

static inline void swap(struct vheap *h, unsigned a, unsigned b)
{
	struct blkhdr *tmp;
	tmp = h->a[a];
	h->a[a] = h->a[b];
	h->a[b] = tmp;
}

static inline void siftup(struct vheap *h, unsigned i)
{
	unsigned j;
	while (i) {
		j = parent(i);
		if (h->a[j] > h->a[i])
			swap(h, j, i);
		else
			break;
		i = j;
	}
}

static void put(struct vheap *h, struct blkhdr *i)
{
	if (h->n == h->cap) {
		unsigned newcap = h->cap << 1;
		struct blkhdr **blk = realloc(heap.a, newcap * sizeof(struct blkhdr*));
		if (!blk) {
			fputs("vmem: can't resize virtual heap: out of memory\n", stderr);
			exit(1);
		}
		h->a = blk;
		h->cap = newcap;
	}
	h->a[h->n] = i;
	siftup(h, h->n++);
}

static inline void sift(struct vheap *h, unsigned i)
{
	unsigned l, r, m;
	r = right(i);
	l = left(i);
	m = i;
	if (l < h->n && h->a[l] < h->a[m])
		m = l;
	if (r < h->n && h->a[r] < h->a[m])
		m = r;
	if (m != i) {
		swap(h, i, m);
		sift(h, m);
	}
}

static inline void del(struct vheap *h, struct blkhdr **ret)
{
	*ret = h->a[0];
	h->a[0] = h->a[--h->n];
	sift(h, 0);
}

void meminit(void)
{
	srand(time(NULL));
	can_mask ^= ((unsigned long)rand() << 32) | rand();
	unsigned cap = 1024;
	struct blkhdr **a = malloc(cap * sizeof(struct blkhdr*));
	if (!a) {
		fputs("out of memory\n", stderr);
		abort();
	}
	heap.a = a;
	heap.n = 0;
	heap.cap = cap;
}

static inline int find(struct vheap *h, void *ptr, struct blkhdr **ret, unsigned *j)
{
	unsigned i = 0;
	// this is the address we are really looking for
	struct blkhdr *ptrp = (struct blkhdr*)((char*)ptr - sizeof(struct blkhdr));
	while (i < h->n) {
		if (h->a[i] == ptrp) {
			*ret = h->a[i];
			*j = i;
			return 1;
		}
		if (h->a[i] > ptrp)
			i = left(i);
		else
			i = right(i);
	}
	return 0;
}

void *vnew(size_t n, const char *file, const char *func, size_t lineno, const char *desc)
{
	void *blk;
	size_t np = sizeof(struct blkhdr) + sizeof(size_t);
	blk = malloc(n + np);
	if (!blk) return NULL;
	// initialize blk
	struct blkhdr *hdr = blk;
	strncpy(hdr->file, file, FILESZ);
	strncpy(hdr->func, func, FUNCSZ);
	if (desc)
		strncpy(hdr->desc, desc, DESCSZ);
	else
		hdr->desc[0] = '\0';
	hdr->file[FILESZ - 1] = '\0';
	hdr->func[FUNCSZ - 1] = '\0';
	hdr->desc[DESCSZ - 1] = '\0';
	hdr->lineno = lineno;
	hdr->size = n;
	// install canaries
	hdr->canary = can_mask;
	*((size_t*)((char*)blk + n + sizeof(struct blkhdr))) = can_mask;
	put(&heap, hdr);
	return (char*)blk + sizeof(struct blkhdr);
}

static void blkchk(struct blkhdr *blk)
{
	if (blk->canary == can_mask && *((size_t*)((char*)blk + blk->size + sizeof(struct blkhdr))) == can_mask)
		return;
	blk->file[FILESZ - 1] = '\0';
	blk->func[FUNCSZ - 1] = '\0';
	blk->desc[DESCSZ - 1] = '\0';
	fprintf(stderr, "vmem: memory corruption: %p\n", (void*)((char*)blk + sizeof(struct blkhdr)));
	fprintf(stderr, "vmem: allocated by %s at %s:%zu\n", blk->file, blk->func, blk->lineno);
	if (blk->desc)
		fprintf(stderr, "vmem: notes: %s\n", blk->desc);
	abort();
}

static void pop(struct vheap *h, unsigned i)
{
	if (!h->n) {
		fputs("vmem: internal error: underflow\n", stderr);
		abort();
	}
	h->a[i] = NULL;
	siftup(h, i);
	h->a[0] = h->a[--h->n];
	sift(h, 0);
}

void nuke(void *ptr, const char *file, const char *func, size_t lineno, const char *desc)
{
	if (!ptr) return;
	struct blkhdr *blk;
	unsigned pos;
	if (!find(&heap, ptr, &blk, &pos)) {
		fprintf(stderr, "vmem: bad ptr or already freed: %p\n", ptr);
		fprintf(stderr, "vmem: called from %s at %s:%zu\n", file, func, lineno);
		if (desc)
			fprintf(stderr, "vmem: notes: %s\n", desc);
		exit(1);
	}
	blkchk(blk);
	pop(&heap, pos);
	free(blk);
}

void memchk(void)
{
	for (unsigned i = 0; i < heap.n; ++i)
		blkchk(heap.a[i]);
}

static inline void blkdump(const struct blkhdr *this)
{
	void *ptr = (char*)this + sizeof(struct blkhdr);
	fprintf(stderr, "%p: %8zX bytes by %s at %s:%zu\n", ptr, this->size, this->file, this->func, this->lineno);
	if (*this->desc)
		fprintf(stderr, "%p: %s\n", ptr, this->desc);
}

#ifdef DEBUG
void memstat(void)
{
	fprintf(stderr, "vmem: allocated blocks: %u\n", heap.n);
	for (unsigned i = 0; i < heap.n; ++i)
		blkdump(heap.a[i]);
}
#endif

void memfree(void)
{
	if (heap.a) {
		if (heap.n) {
			fprintf(stderr, "vmem: leaked blocks: %u\n", heap.n);
			for (unsigned i = 0; i < heap.n; ++i) {
				blkdump(heap.a[i]);
				free(heap.a[i]);
			}
		}
		free(heap.a);
		heap.a = NULL;
	}
}
