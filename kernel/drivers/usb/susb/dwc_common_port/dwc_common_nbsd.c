#include "dwc_os.h"
#include "dwc_list.h"

#ifdef DWC_CCLIB
# include "dwc_cc.h"
#endif

#ifdef DWC_CRYPTOLIB
# include "dwc_modpow.h"
# include "dwc_dh.h"
# include "dwc_crypto.h"
#endif

#ifdef DWC_NOTIFYLIB
# include "dwc_notifier.h"
#endif

/* OS-Level Implementations */

/* This is the NetBSD 4.0.1 kernel implementation of the DWC platform library. */


/* MISC */

void *DWC_MEMSET(void *dest, uint8_t byte, uint32_t size)
{
	return memset(dest, byte, size);
}

void *DWC_MEMCPY(void *dest, void const *src, uint32_t size)
{
	return memcpy(dest, src, size);
}

void *DWC_MEMMOVE(void *dest, void *src, uint32_t size)
{
	bcopy(src, dest, size);
	return dest;
}

int DWC_MEMCMP(void *m1, void *m2, uint32_t size)
{
	return memcmp(m1, m2, size);
}

int DWC_STRNCMP(void *s1, void *s2, uint32_t size)
{
	return strncmp(s1, s2, size);
}

int DWC_STRCMP(void *s1, void *s2)
{
	return strcmp(s1, s2);
}

int DWC_STRLEN(char const *str)
{
	return strlen(str);
}

char *DWC_STRCPY(char *to, char const *from)
{
	return strcpy(to, from);
}

char *DWC_STRDUP(char const *str)
{
	int len = DWC_STRLEN(str) + 1;
	char *new = DWC_ALLOC_ATOMIC(len);

	if (!new) {
		return NULL;
	}

	DWC_MEMCPY(new, str, len);
	return new;
}

int DWC_ATOI(char *str, int32_t *value)
{
	char *end = NULL;

	/* NetBSD doesn't have 'strtol' in the kernel, but 'strtoul'
	 * should be equivalent on 2's complement machines
	 */
	*value = strtoul(str, &end, 0);
	if (*end == '\0') {
		return 0;
	}

	return -1;
}

int DWC_ATOUI(char *str, uint32_t *value)
{
	char *end = NULL;

	*value = strtoul(str, &end, 0);
	if (*end == '\0') {
		return 0;
	}

	return -1;
}


#ifdef DWC_UTFLIB
/* From usbstring.c */

int DWC_UTF8_TO_UTF16LE(uint8_t const *s, uint16_t *cp, unsigned len)
{
	int	count = 0;
	u8	c;
	u16	uchar;

	/* this insists on correct encodings, though not minimal ones.
	 * BUT it currently rejects legit 4-byte UTF-8 code points,
	 * which need surrogate pairs.  (Unicode 3.1 can use them.)
	 */
	while (len != 0 && (c = (u8) *s++) != 0) {
		if (unlikely(c & 0x80)) {
			/* 2-byte sequence: */
			/* 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx */
			if ((c & 0xe0) == 0xc0) {
				uchar = (c & 0x1f) << 6;

				c = (u8) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

				/* 3-byte sequence (most CJKV characters): */
				/* zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx */
			} else if ((c & 0xf0) == 0xe0) {
				uchar = (c & 0x0f) << 12;

				c = (u8) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c << 6;

				c = (u8) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

				/* no bogus surrogates */
				if (0xd800 <= uchar && uchar <= 0xdfff)
					goto fail;

				/** 4-byte sequence (surrogate pairs, currently rare):
				* 11101110wwwwzzzzyy + 110111yyyyxxxxxx
				*     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
				* (uuuuu = wwww + 1)
				* FIXME accept the surrogate code points (only) */
			} else
				goto fail;
		} else
			uchar = c;
		put_unaligned(cpu_to_le16(uchar), cp++);
		count++;
		len--;
	}
	return count;
fail:
	return -1;
}

#endif	/* DWC_UTFLIB */


/* dwc_debug.h */

dwc_bool_t DWC_IN_IRQ(void)
{
	return 0;
}

dwc_bool_t DWC_IN_BH(void)
{
	return 0;
}

void DWC_VPRINTF(char *format, va_list args)
{
	vprintf(format, args);
}

int DWC_VSNPRINTF(char *str, int size, char *format, va_list args)
{
	return vsnprintf(str, size, format, args);
}

void DWC_PRINTF(char *format, ...)
{
	va_list args;

	va_start(args, format);
	DWC_VPRINTF(format, args);
	va_end(args);
}

int DWC_SPRINTF(char *buffer, char *format, ...)
{
	int retval;
	va_list args;

	va_start(args, format);
	retval = vsprintf(buffer, format, args);
	va_end(args);
	return retval;
}

int DWC_SNPRINTF(char *buffer, int size, char *format, ...)
{
	int retval;
	va_list args;

	va_start(args, format);
	retval = vsnprintf(buffer, size, format, args);
	va_end(args);
	return retval;
}

void __DWC_WARN(char *format, ...)
{
	va_list args;

	va_start(args, format);
	DWC_VPRINTF(format, args);
	va_end(args);
}

void __DWC_ERROR(char *format, ...)
{
	va_list args;

	va_start(args, format);
	DWC_VPRINTF(format, args);
	va_end(args);
}

void DWC_EXCEPTION(char *format, ...)
{
	va_list args;

	va_start(args, format);
	DWC_VPRINTF(format, args);
	va_end(args);
}

#ifdef DEBUG
void __DWC_DEBUG(char *format, ...)
{
	va_list args;

	va_start(args, format);
	DWC_VPRINTF(format, args);
	va_end(args);
}
#endif


/* dwc_mem.h */

void *__DWC_DMA_ALLOC(void *dma_ctx, uint32_t size, dwc_dma_t *dma_addr)
{
	dwc_dmactx_t *dma = (dwc_dmactx_t *)dma_ctx;
	int error;

	error = bus_dmamem_alloc(dma->dma_tag, size, 1, size, dma->segs,
				 sizeof(dma->segs) / sizeof(dma->segs[0]),
				 &dma->nsegs, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: bus_dmamem_alloc(%ju) failed: %d\n", __func__,
			   (uintmax_t)size, error);
		goto fail_0;
	}

	error = bus_dmamem_map(dma->dma_tag, dma->segs, dma->nsegs, size,
				(caddr_t *)&dma->dma_vaddr,
				BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		printf("%s: bus_dmamem_map failed: %d\n", __func__, error);
		goto fail_1;
	}

	error = bus_dmamap_create(dma->dma_tag, size, 1, size, 0,
				  BUS_DMA_NOWAIT, &dma->dma_map);
	if (error) {
		printf("%s: bus_dmamap_create failed: %d\n", __func__, error);
		goto fail_2;
	}

	error = bus_dmamap_load(dma->dma_tag, dma->dma_map, dma->dma_vaddr,
				size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: bus_dmamap_load failed: %d\n", __func__, error);
		goto fail_3;
	}

	dma->dma_paddr = (bus_addr_t)dma->segs[0].ds_addr;
	*dma_addr = dma->dma_paddr;
	return dma->dma_vaddr;

fail_3:
	bus_dmamap_destroy(dma->dma_tag, dma->dma_map);
fail_2:
	bus_dmamem_unmap(dma->dma_tag, dma->dma_vaddr, size);
fail_1:
	bus_dmamem_free(dma->dma_tag, dma->segs, dma->nsegs);
fail_0:
	dma->dma_map = NULL;
	dma->dma_vaddr = NULL;
	dma->nsegs = 0;

	return NULL;
}

void __DWC_DMA_FREE(void *dma_ctx, uint32_t size, void *virt_addr, dwc_dma_t dma_addr)
{
	dwc_dmactx_t *dma = (dwc_dmactx_t *)dma_ctx;

	if (dma->dma_map != NULL) {
		bus_dmamap_sync(dma->dma_tag, dma->dma_map, 0, size,
				BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dma->dma_tag, dma->dma_map);
		bus_dmamap_destroy(dma->dma_tag, dma->dma_map);
		bus_dmamem_unmap(dma->dma_tag, dma->dma_vaddr, size);
		bus_dmamem_free(dma->dma_tag, dma->segs, dma->nsegs);
		dma->dma_paddr = 0;
		dma->dma_map = NULL;
		dma->dma_vaddr = NULL;
		dma->nsegs = 0;
	}
}

void *__DWC_ALLOC(void *mem_ctx, uint32_t size)
{
	return malloc(size, M_DEVBUF, M_WAITOK | M_ZERO);
}

void *__DWC_ALLOC_ATOMIC(void *mem_ctx, uint32_t size)
{
	return malloc(size, M_DEVBUF, M_NOWAIT | M_ZERO);
}

void __DWC_FREE(void *mem_ctx, void *addr)
{
	free(addr, M_DEVBUF);
}


#ifdef DWC_CRYPTOLIB
/* dwc_crypto.h */

void DWC_RANDOM_BYTES(uint8_t *buffer, uint32_t length)
{
	get_random_bytes(buffer, length);
}

int DWC_AES_CBC(uint8_t *message, uint32_t messagelen, uint8_t *key, uint32_t keylen, uint8_t iv[16], uint8_t *out)
{
	struct crypto_blkcipher *tfm;
	struct blkcipher_desc desc;
	struct scatterlist sgd;
	struct scatterlist sgs;

	tfm = crypto_alloc_blkcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC);
	if (tfm == NULL) {
		printk("failed to load transform for aes CBC\n");
		return -1;
	}

	crypto_blkcipher_setkey(tfm, key, keylen);
	crypto_blkcipher_set_iv(tfm, iv, 16);

	sg_init_one(&sgd, out, messagelen);
	sg_init_one(&sgs, message, messagelen);

	desc.tfm = tfm;
	desc.flags = 0;

	if (crypto_blkcipher_encrypt(&desc, &sgd, &sgs, messagelen)) {
		crypto_free_blkcipher(tfm);
		DWC_ERROR("AES CBC encryption failed");
		return -1;
	}

	crypto_free_blkcipher(tfm);
	return 0;
}

int DWC_SHA256(uint8_t *message, uint32_t len, uint8_t *out)
{
	struct crypto_hash *tfm;
	struct hash_desc desc;
	struct scatterlist sg;

	tfm = crypto_alloc_hash("sha256", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		DWC_ERROR("Failed to load transform for sha256: %ld", PTR_ERR(tfm));
		return 0;
	}
	desc.tfm = tfm;
	desc.flags = 0;

	sg_init_one(&sg, message, len);
	crypto_hash_digest(&desc, &sg, len, out);
	crypto_free_hash(tfm);

	return 1;
}

int DWC_HMAC_SHA256(uint8_t *message, uint32_t messagelen,
		    uint8_t *key, uint32_t keylen, uint8_t *out)
{
	struct crypto_hash *tfm;
	struct hash_desc desc;
	struct scatterlist sg;

	tfm = crypto_alloc_hash("hmac(sha256)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		DWC_ERROR("Failed to load transform for hmac(sha256): %ld", PTR_ERR(tfm));
		return 0;
	}
	desc.tfm = tfm;
	desc.flags = 0;

	sg_init_one(&sg, message, messagelen);
	crypto_hash_setkey(tfm, key, keylen);
	crypto_hash_digest(&desc, &sg, messagelen, out);
	crypto_free_hash(tfm);

	return 1;
}

#endif	/* DWC_CRYPTOLIB */


/* Byte Ordering Conversions */

uint32_t DWC_CPU_TO_LE32(uint32_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24);
#endif
}

uint32_t DWC_CPU_TO_BE32(uint32_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24);
#endif
}

uint32_t DWC_LE32_TO_CPU(uint32_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24);
#endif
}

uint32_t DWC_BE32_TO_CPU(uint32_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24);
#endif
}

uint16_t DWC_CPU_TO_LE16(uint16_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return u_p[1] | (u_p[0] << 8);
#endif
}

uint16_t DWC_CPU_TO_BE16(uint16_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return u_p[1] | (u_p[0] << 8);
#endif
}

uint16_t DWC_LE16_TO_CPU(uint16_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return u_p[1] | (u_p[0] << 8);
#endif
}

uint16_t DWC_BE16_TO_CPU(uint16_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return u_p[1] | (u_p[0] << 8);
#endif
}


/* Registers */

uint32_t DWC_READ_REG32(void *io_ctx, uint32_t volatile * reg)
{
	dwc_ioctx_t *io = (dwc_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	return bus_space_read_4(io->iot, io->ioh, ior);
}

void DWC_WRITE_REG32(void *io_ctx, uint32_t volatile * reg, uint32_t value)
{
	dwc_ioctx_t *io = (dwc_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	bus_space_write_4(io->iot, io->ioh, ior, value);
}

void DWC_MODIFY_REG32(void *io_ctx, uint32_t volatile * reg, uint32_t clear_mask,
			uint32_t set_mask)
{
	dwc_ioctx_t *io = (dwc_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	bus_space_write_4(io->iot, io->ioh, ior,
			(bus_space_read_4(io->iot, io->ioh, ior) &
			~clear_mask) | set_mask);
}

dwc_spinlock_t *DWC_SPINLOCK_ALLOC(void)
{
	struct simplelock *sl = DWC_ALLOC(sizeof(*sl));

	if (!sl) {
		DWC_ERROR("Cannot allocate memory for spinlock");
		return NULL;
	}

	simple_lock_init(sl);
	return (dwc_spinlock_t *)sl;
}

void DWC_SPINLOCK_FREE(dwc_spinlock_t *lock)
{
	struct simplelock *sl = (struct simplelock *)lock;

	DWC_FREE(sl);
}

void DWC_SPINLOCK(dwc_spinlock_t *lock)
{
	simple_lock((struct simplelock *)lock);
}

void DWC_SPINUNLOCK(dwc_spinlock_t *lock)
{
	simple_unlock((struct simplelock *)lock);
}

void DWC_SPINLOCK_IRQSAVE(dwc_spinlock_t *lock, dwc_irqflags_t *flags)
{
	simple_lock((struct simplelock *)lock);
	*flags = splbio();
}

void DWC_SPINUNLOCK_IRQRESTORE(dwc_spinlock_t *lock, dwc_irqflags_t flags)
{
	splx(flags);
	simple_unlock((struct simplelock *)lock);
}

dwc_mutex_t *DWC_MUTEX_ALLOC(void)
{
	dwc_mutex_t *mutex = DWC_ALLOC(sizeof(struct lock));

	if (!mutex) {
		DWC_ERROR("Cannot allocate memory for mutex");
		return NULL;
	}

	lockinit((struct lock *)mutex, 0, "dw3mtx", 0, 0);
	return mutex;
}

#if (defined(DWC_LINUX) && defined(CONFIG_DEBUG_MUTEXES))
#else
void DWC_MUTEX_FREE(dwc_mutex_t *mutex)
{
	DWC_FREE(mutex);
}
#endif

void DWC_MUTEX_LOCK(dwc_mutex_t *mutex)
{
	lockmgr((struct lock *)mutex, LK_EXCLUSIVE, NULL);
}

int DWC_MUTEX_TRYLOCK(dwc_mutex_t *mutex)
{
	int status;

	status = lockmgr((struct lock *)mutex, LK_EXCLUSIVE | LK_NOWAIT, NULL);
	return status == 0;
}

void DWC_MUTEX_UNLOCK(dwc_mutex_t *mutex)
{
	lockmgr((struct lock *)mutex, LK_RELEASE, NULL);
}


/* Timing */

void DWC_UDELAY(uint32_t usecs)
{
	DELAY(usecs);
}

void DWC_MDELAY(uint32_t msecs)
{
	do {
		DELAY(1000);
	} while (--msecs);
}

void DWC_MSLEEP(uint32_t msecs)
{
	struct timeval tv;

	tv.tv_sec = msecs / 1000;
	tv.tv_usec = (msecs - tv.tv_sec * 1000) * 1000;
	tsleep(&tv, 0, "dw3slp", tvtohz(&tv));
}

uint32_t DWC_TIME(void)
{
	struct timeval tv;

	microuptime(&tv);	/* or getmicrouptime? (less precise, but faster) */
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


/* Timers */

struct dwc_timer {
	struct callout t;
	char *name;
	dwc_spinlock_t *lock;
	dwc_timer_callback_t cb;
	void *data;
};

dwc_timer_t *DWC_TIMER_ALLOC(char *name, dwc_timer_callback_t cb, void *data)
{
	dwc_timer_t *t = DWC_ALLOC(sizeof(*t));

	if (!t) {
		DWC_ERROR("Cannot allocate memory for timer");
		return NULL;
	}

	callout_init(&t->t);

	t->name = DWC_STRDUP(name);
	if (!t->name) {
		DWC_ERROR("Cannot allocate memory for timer->name");
		goto no_name;
	}

	t->lock = DWC_SPINLOCK_ALLOC();
	if (!t->lock) {
		DWC_ERROR("Cannot allocate memory for timer->lock");
		goto no_lock;
	}

	t->cb = cb;
	t->data = data;

	return t;

no_lock:
	DWC_FREE(t->name);
no_name:
	DWC_FREE(t);

	return NULL;
}

void DWC_TIMER_FREE(dwc_timer_t *timer)
{
	callout_stop(&timer->t);
	DWC_SPINLOCK_FREE(timer->lock);
	DWC_FREE(timer->name);
	DWC_FREE(timer);
}

void DWC_TIMER_SCHEDULE(dwc_timer_t *timer, uint32_t time)
{
	struct timeval tv;

	tv.tv_sec = time / 1000;
	tv.tv_usec = (time - tv.tv_sec * 1000) * 1000;
	callout_reset(&timer->t, tvtohz(&tv), timer->cb, timer->data);
}

void DWC_TIMER_CANCEL(dwc_timer_t *timer)
{
	callout_stop(&timer->t);
}


/* Wait Queues */

struct dwc_waitq {
	struct simplelock lock;
	int abort;
};

dwc_waitq_t *DWC_WAITQ_ALLOC(void)
{
	dwc_waitq_t *wq = DWC_ALLOC(sizeof(*wq));

	if (!wq) {
		DWC_ERROR("Cannot allocate memory for waitqueue");
		return NULL;
	}

	simple_lock_init(&wq->lock);
	wq->abort = 0;

	return wq;
}

void DWC_WAITQ_FREE(dwc_waitq_t *wq)
{
	DWC_FREE(wq);
}

int32_t DWC_WAITQ_WAIT(dwc_waitq_t *wq, dwc_waitq_condition_t cond, void *data)
{
	int ipl;
	int result = 0;

	simple_lock(&wq->lock);
	ipl = splbio();

	/* Skip the sleep if already aborted or triggered */
	if (!wq->abort && !cond(data)) {
		splx(ipl);
		result = ltsleep(wq, PCATCH, "dw3wat", 0, &wq->lock); /* infinite timeout */
		ipl = splbio();
	}

	if (result == 0) { /* awoken */
		if (wq->abort) {
			wq->abort = 0;
			result = -DWC_E_ABORT;
		} else {
			result = 0;
		}

		splx(ipl);
		simple_unlock(&wq->lock);
	} else {
		wq->abort = 0;
		splx(ipl);
		simple_unlock(&wq->lock);

		if (result == ERESTART) { /* signaled - restart */
			result = -DWC_E_RESTART;
		} else {			/* signaled - must be EINTR */
			result = -DWC_E_ABORT;
		}
	}

	return result;
}

int32_t DWC_WAITQ_WAIT_TIMEOUT(dwc_waitq_t *wq, dwc_waitq_condition_t cond,
				void *data, int32_t msecs)
{
	struct timeval tv, tv1, tv2;
	int ipl;
	int result = 0;

	tv.tv_sec = msecs / 1000;
	tv.tv_usec = (msecs - tv.tv_sec * 1000) * 1000;

	simple_lock(&wq->lock);
	ipl = splbio();

	/* Skip the sleep if already aborted or triggered */
	if (!wq->abort && !cond(data)) {
		splx(ipl);
		getmicrouptime(&tv1);
		result = ltsleep(wq, PCATCH, "dw3wto", tvtohz(&tv), &wq->lock);
		getmicrouptime(&tv2);
		ipl = splbio();
	}

	if (result == 0) { /* awoken */
		if (wq->abort) {
			wq->abort = 0;
			splx(ipl);
			simple_unlock(&wq->lock);
			result = -DWC_E_ABORT;
		} else {
			splx(ipl);
			simple_unlock(&wq->lock);

			tv2.tv_usec -= tv1.tv_usec;
			if (tv2.tv_usec < 0) {
				tv2.tv_usec += 1000000;
				tv2.tv_sec--;
			}

			tv2.tv_sec -= tv1.tv_sec;
			result = tv2.tv_sec * 1000 + tv2.tv_usec / 1000;
			result = msecs - result;
			if (result <= 0)
				result = 1;
		}
	} else {
		wq->abort = 0;
		splx(ipl);
		simple_unlock(&wq->lock);

		if (result == ERESTART) {
			/* signaled - restart */
			result = -DWC_E_RESTART;

		} else if (result == EINTR) {
			/* signaled - interrupt */
			result = -DWC_E_ABORT;

		} else {				/* timed out */
			result = -DWC_E_TIMEOUT;
		}
	}

	return result;
}

void DWC_WAITQ_TRIGGER(dwc_waitq_t *wq)
{
	wakeup(wq);
}

void DWC_WAITQ_ABORT(dwc_waitq_t *wq)
{
	int ipl;

	simple_lock(&wq->lock);
	ipl = splbio();
	wq->abort = 1;
	wakeup(wq);
	splx(ipl);
	simple_unlock(&wq->lock);
}


/* Threading */

struct dwc_thread {
	struct proc *proc;
	int abort;
};

dwc_thread_t *DWC_THREAD_RUN(dwc_thread_function_t func, char *name, void *data)
{
	int retval;
	dwc_thread_t *thread = DWC_ALLOC(sizeof(*thread));

	if (!thread) {
		return NULL;
	}

	thread->abort = 0;
	retval = kthread_create1((void (*)(void *))func, data, &thread->proc,
				"%s", name);
	if (retval) {
		DWC_FREE(thread);
		return NULL;
	}

	return thread;
}

int DWC_THREAD_STOP(dwc_thread_t *thread)
{
	int retval;

	thread->abort = 1;
	retval = tsleep(&thread->abort, 0, "dw3stp", 60 * hz);

	if (retval == 0) {
		/* DWC_THREAD_EXIT() will free the thread struct */
		return 0;
	}

	/* NOTE: We leak the thread struct if thread doesn't die */

	if (retval == EWOULDBLOCK) {
		return -DWC_E_TIMEOUT;
	}

	return -DWC_E_UNKNOWN;
}

dwc_bool_t DWC_THREAD_SHOULD_STOP(dwc_thread_t *thread)
{
	return thread->abort;
}

void DWC_THREAD_EXIT(dwc_thread_t *thread)
{
	wakeup(&thread->abort);
	DWC_FREE(thread);
	kthread_exit(0);
}

/* tasklets
 - Runs in interrupt context (cannot sleep)
 - Each tasklet runs on a single CPU
 - Different tasklets can be running simultaneously on different CPUs
 [ On NetBSD there is no corresponding mechanism, drivers don't have bottom-
   halves. So we just call the callback directly from DWC_TASK_SCHEDULE() ]
 */
struct dwc_tasklet {
	dwc_tasklet_callback_t cb;
	void *data;
};

static void tasklet_callback(void *data)
{
	dwc_tasklet_t *task = (dwc_tasklet_t *)data;

	task->cb(task->data);
}

dwc_tasklet_t *DWC_TASK_ALLOC(char *name, dwc_tasklet_callback_t cb, void *data)
{
	dwc_tasklet_t *task = DWC_ALLOC(sizeof(*task));

	if (task) {
		task->cb = cb;
		task->data = data;
	} else {
		DWC_ERROR("Cannot allocate memory for tasklet");
	}

	return task;
}

void DWC_TASK_FREE(dwc_tasklet_t *task)
{
	DWC_FREE(task);
}

void DWC_TASK_SCHEDULE(dwc_tasklet_t *task)
{
	tasklet_callback(task);
}


/* workqueues
 - Runs in process context (can sleep)
 */
typedef struct work_container {
	dwc_work_callback_t cb;
	void *data;
	dwc_workq_t *wq;
	char *name;
	int hz;
	struct work task;
} work_container_t;

struct dwc_workq {
	struct workqueue *taskq;
	dwc_spinlock_t *lock;
	dwc_waitq_t *waitq;
	int pending;
	struct work_container *container;
};

static void do_work(struct work *task, void *data)
{
	dwc_workq_t *wq = (dwc_workq_t *)data;
	work_container_t *container = wq->container;
	dwc_irqflags_t flags;

	if (container->hz) {
		tsleep(container, 0, "dw3wrk", container->hz);
	}

	container->cb(container->data);
	DWC_DEBUG("Work done: %s, container=%p", container->name, container);

	DWC_SPINLOCK_IRQSAVE(wq->lock, &flags);
	if (container->name)
		DWC_FREE(container->name);
	DWC_FREE(container);
	wq->pending--;
	DWC_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
	DWC_WAITQ_TRIGGER(wq->waitq);
}

static int work_done(void *data)
{
	dwc_workq_t *workq = (dwc_workq_t *)data;

	return workq->pending == 0;
}

int DWC_WORKQ_WAIT_WORK_DONE(dwc_workq_t *workq, int timeout)
{
	return DWC_WAITQ_WAIT_TIMEOUT(workq->waitq, work_done, workq, timeout);
}

dwc_workq_t *DWC_WORKQ_ALLOC(char *name)
{
	int result;
	dwc_workq_t *wq = DWC_ALLOC(sizeof(*wq));

	if (!wq) {
		DWC_ERROR("Cannot allocate memory for workqueue");
		return NULL;
	}

	result = workqueue_create(&wq->taskq, name, do_work, wq, 0 /*PWAIT*/,
				IPL_BIO, 0);
	if (result) {
		DWC_ERROR("Cannot create workqueue");
		goto no_taskq;
	}

	wq->pending = 0;

	wq->lock = DWC_SPINLOCK_ALLOC();
	if (!wq->lock) {
		DWC_ERROR("Cannot allocate memory for spinlock");
		goto no_lock;
	}

	wq->waitq = DWC_WAITQ_ALLOC();
	if (!wq->waitq) {
		DWC_ERROR("Cannot allocate memory for waitqueue");
		goto no_waitq;
	}

	return wq;

no_waitq:
	DWC_SPINLOCK_FREE(wq->lock);
no_lock:
	workqueue_destroy(wq->taskq);
no_taskq:
	DWC_FREE(wq);

	return NULL;
}

void DWC_WORKQ_FREE(dwc_workq_t *wq)
{
#ifdef DEBUG
	dwc_irqflags_t flags;

	DWC_SPINLOCK_IRQSAVE(wq->lock, &flags);

	if (wq->pending != 0) {
		struct work_container *container = wq->container;

		DWC_ERROR("Destroying work queue with pending work");

		if (container && container->name) {
			DWC_ERROR("Work %s still pending", container->name);
		}
	}

	DWC_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
#endif
	DWC_WAITQ_FREE(wq->waitq);
	DWC_SPINLOCK_FREE(wq->lock);
	workqueue_destroy(wq->taskq);
	DWC_FREE(wq);
}

void DWC_WORKQ_SCHEDULE(dwc_workq_t *wq, dwc_work_callback_t cb, void *data,
			char *format, ...)
{
	dwc_irqflags_t flags;
	work_container_t *container;
	static char name[128];
	va_list args;

	va_start(args, format);
	DWC_VSNPRINTF(name, 128, format, args);
	va_end(args);

	DWC_SPINLOCK_IRQSAVE(wq->lock, &flags);
	wq->pending++;
	DWC_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
	DWC_WAITQ_TRIGGER(wq->waitq);

	container = DWC_ALLOC_ATOMIC(sizeof(*container));
	if (!container) {
		DWC_ERROR("Cannot allocate memory for container");
		return;
	}

	container->name = DWC_STRDUP(name);
	if (!container->name) {
		DWC_ERROR("Cannot allocate memory for container->name");
		DWC_FREE(container);
		return;
	}

	container->cb = cb;
	container->data = data;
	container->wq = wq;
	container->hz = 0;
	wq->container = container;

	DWC_DEBUG("Queueing work: %s, container=%pK", container->name, container);
	workqueue_enqueue(wq->taskq, &container->task);
}

void DWC_WORKQ_SCHEDULE_DELAYED(dwc_workq_t *wq, dwc_work_callback_t cb,
				void *data, uint32_t time, char *format, ...)
{
	dwc_irqflags_t flags;
	work_container_t *container;
	static char name[128];
	struct timeval tv;
	va_list args;

	va_start(args, format);
	DWC_VSNPRINTF(name, 128, format, args);
	va_end(args);

	DWC_SPINLOCK_IRQSAVE(wq->lock, &flags);
	wq->pending++;
	DWC_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
	DWC_WAITQ_TRIGGER(wq->waitq);

	container = DWC_ALLOC_ATOMIC(sizeof(*container));
	if (!container) {
		DWC_ERROR("Cannot allocate memory for container");
		return;
	}

	container->name = DWC_STRDUP(name);
	if (!container->name) {
		DWC_ERROR("Cannot allocate memory for container->name");
		DWC_FREE(container);
		return;
	}

	container->cb = cb;
	container->data = data;
	container->wq = wq;
	tv.tv_sec = time / 1000;
	tv.tv_usec = (time - tv.tv_sec * 1000) * 1000;
	container->hz = tvtohz(&tv);
	wq->container = container;

	DWC_DEBUG("Queueing work: %s, container=%pK", container->name, container);
	workqueue_enqueue(wq->taskq, &container->task);
}

int DWC_WORKQ_PENDING(dwc_workq_t *wq)
{
	return wq->pending;
}
