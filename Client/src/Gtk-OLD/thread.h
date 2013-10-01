#ifndef THREAD_H
#define THREAD_H

	#define THREAD_CC
	#define THREAD_TYPE                    pthread_t
	#define THREAD_CREATE(tid, entry, arg) pthread_create(&(tid), NULL, \
                                                    (entry), &(arg))
#endif /* THREAD_H */
