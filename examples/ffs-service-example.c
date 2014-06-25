/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#define _BSD_SOURCE /* for endian.h */

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/eventfd.h>

#include "libaio.h"
#define IOCB_FLAG_RESFD         (1 << 0)

#include <linux/usb/functionfs.h>
#include <gadgetd/ffs-daemon.h>

#define BUF_LEN		8192

/******************** Descriptors and Strings *******************************/

/*
 * Descriptors and strings are taken by gadgetd from service file,
 * so they are not necessary here.
 */

/******************** Endpoints handling *******************************/

static void display_event(struct usb_functionfs_event *event)
{
	static const char *const names[] = {
		[FUNCTIONFS_BIND] = "BIND",
		[FUNCTIONFS_UNBIND] = "UNBIND",
		[FUNCTIONFS_ENABLE] = "ENABLE",
		[FUNCTIONFS_DISABLE] = "DISABLE",
		[FUNCTIONFS_SETUP] = "SETUP",
		[FUNCTIONFS_SUSPEND] = "SUSPEND",
		[FUNCTIONFS_RESUME] = "RESUME",
	};
	switch (event->type) {
	case FUNCTIONFS_BIND:
	case FUNCTIONFS_UNBIND:
	case FUNCTIONFS_ENABLE:
	case FUNCTIONFS_DISABLE:
	case FUNCTIONFS_SETUP:
	case FUNCTIONFS_SUSPEND:
	case FUNCTIONFS_RESUME:
		printf("Event %s\n", names[event->type]);
	}
}

static void handle_ep0(int ep0, bool *ready)
{
	struct usb_functionfs_event event;
	int ret;

	struct pollfd pfds[1];
	pfds[0].fd = ep0;
	pfds[0].events = POLLIN;

	ret = poll(pfds, 1, 0);

	if (ret && (pfds[0].revents & POLLIN)) {
		ret = read(ep0, &event, sizeof(event));
		if (!ret) {
			perror("unable to read event from ep0");
			return;
		}
		display_event(&event);
		switch (event.type) {
		case FUNCTIONFS_SETUP:
			if (event.u.setup.bRequestType & USB_DIR_IN)
				write(ep0, NULL, 0);
			else
				read(ep0, NULL, 0);
			break;

		case FUNCTIONFS_ENABLE:
			*ready = true;
			break;

		case FUNCTIONFS_DISABLE:
			*ready = false;
			break;

		default:
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int i, ret;
	int ep[3];
	io_context_t io_ctx;
	int evfd;
	fd_set rfds;
	char buf_in[BUF_LEN], buf_out[BUF_LEN];
	struct iocb iocb_in, iocb_out;
	int req_in = 0, req_out = 0;
	bool ready;
	enum usb_functionfs_event_type e;

	/*
	 * We don't need to open any file descriptors because they are
	 * prepared for us, just take them using gadgetd library.
	 */

	/* This is simple example so we assume that
	 * 3 descriptors should be passed */
	ret = gd_nmb_of_ep(0);
	if (ret != 3) {
		perror("Incompatible number of ep received");
		return 1;
	}

	for (i = 0; i < 3; ++i) {
		ep[i] = gd_get_ep_by_nmb(i);
		if (ep[i] < 0) {
			perror("Unable to get descriptors");
			return 1;
		}
	}

	/* Check what was our activation event */
	e = gd_get_activation_event(0);
	switch (e) {
	case FUNCTIONFS_BIND:
		ready = false;
		break;
	case FUNCTIONFS_ENABLE:
	case FUNCTIONFS_SETUP:
		ready = true;
		break;
	default:
		perror("Error or unsupported activation event");
		return 1;
	}

	memset(&io_ctx, 0, sizeof(io_ctx));
	/* setup aio context to handle up to 2 requests */
	if (io_setup(2, &io_ctx) < 0) {
		perror("unable to setup aio");
		return 1;
	}

	evfd = eventfd(0, 0);
	if (evfd < 0) {
		perror("unable to open eventfd");
		return 1;
	}

	/* If we are ready we skip initial select */
	if (ready)
		goto queue_requests;

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(ep[0], &rfds);
		FD_SET(evfd, &rfds);

		ret = select(((ep[0] > evfd) ? ep[0] : evfd)+1,
			     &rfds, NULL, NULL, NULL);

		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("select");
			break;
		}

		if (FD_ISSET(ep[0], &rfds))
			handle_ep0(ep[0], &ready);

		/* we are waiting for function ENABLE */
		if (!ready)
			continue;

		/* if something was submitted we wait for event */
		if (FD_ISSET(evfd, &rfds)) {
			uint64_t ev_cnt;
			ret = read(evfd, &ev_cnt, sizeof(ev_cnt));
			if (ret < 0) {
				perror("unable to read eventfd");
				break;
			}

			struct io_event e[2];
			/* we wait for one event */
			ret = io_getevents(io_ctx, 1, 2, e, NULL);
			/* if we got event */
			for (i = 0; i < ret; ++i) {
				if (e[i].obj->aio_fildes == ep[1]) {
					printf("ev=in; ret=%lu\n", e[i].res);
					req_in = 0;
				} else if (e[i].obj->aio_fildes == ep[2]) {
					printf("ev=out; ret=%lu\n", e[i].res);
					req_out = 0;
				}
			}
		}

queue_requests:
		if (!req_in) { /* if IN transfer not requested*/
			struct iocb *iocb = &iocb_in;
			/* prepare request */
			io_prep_pwrite(iocb, ep[1], buf_in, BUF_LEN, 0);
			/* enable eventfs notification */
			iocb->u.c.flags |= IOCB_FLAG_RESFD;
			iocb->u.c.resfd = evfd;
			/* submit table of requests */
			ret = io_submit(io_ctx, 1, &iocb);
			if (ret >= 0) { /* if ret > 0 request is queued */
				req_in = 1;
				printf("submit: in\n");
			} else
				perror("unable to submit request");
		}

		if (!req_out) { /* if OUT transfer not requested */
			struct iocb *iocb = &iocb_out;
			/* prepare request */
			io_prep_pread(iocb, ep[2], buf_out, BUF_LEN, 0);
			/* enable eventfs notification */
			iocb->u.c.flags |= IOCB_FLAG_RESFD;
			iocb->u.c.resfd = evfd;
			/* submit table of requests */
			ret = io_submit(io_ctx, 1, &iocb);
			if (ret >= 0) { /* if ret > 0 request is queued */
				req_out = 1;
				printf("submit: out\n");
			} else
				perror("unable to submit request");
		}
	}

	/* free resources */
	io_destroy(io_ctx);

	for (i = 0; i < 3; ++i)
		close(ep[i]);

	return 0;
}

