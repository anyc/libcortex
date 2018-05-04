/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "core.h"

#include <libetpan/libetpan.h>

void crtx_mail_init() {
}

void crtx_mail_finish() {
}


// OBJS+=mail.o
// CFLAGS+=$(shell libetpan-config --cflags)
// LDLIBS+=$(shell libetpan-config --libs)

#ifdef CRTX_TEST
int mail_main(int argc, char **argv) {
	int r;
	char *path = "INBOX";
	struct mailstorage * storage;
	struct mailfolder * folder;
	
	mailstream_debug=1;

	storage = mailstorage_new(NULL);
	if (storage == NULL) {
		printf("error initializing storage\n");
// 		goto free_opt;
	}
	
	r = imap_mailstorage_init(storage, "mail.zeus06.de", 143, NULL, CONNECTION_TYPE_STARTTLS,
		IMAP_AUTH_TYPE_PLAIN, "l3s6476p3", "password", 0, NULL);
	if (r != MAIL_NO_ERROR) {
		printf("error initializing storage\n");
// 		goto free_opt;
	}
	printf("asd\n");
	/* get the folder structure */

	folder = mailfolder_new(storage, path, NULL);
	if (folder == NULL) {
		printf("error initializing folder\n");
// 		goto free_storage;
	}
printf("asd2\n");
	r = mailfolder_connect(folder);
	if (r != MAIL_NO_ERROR) {
		printf("error initializing folder\n");
// 		goto free_folder;
	}
	printf("asd3\n");
// 	while (optind < argc) {
		mailmessage * msg;
		uint32_t msg_num;
// 		struct mailmime * mime;
		char *data;
		size_t size;

// 		msg_num = strtoul(argv[optind], NULL, 10);
		msg_num = 0;
		
		uint32_t recent;
// 		mailsession_messages_number(mailsession * session, const char * mb,
// 				uint32_t * result);
		mailsession_messages_number(folder->fld_session,
			      path, &recent);
		printf("recent %d\n", recent);
		msg_num = recent;
		
		r = mailsession_get_message(folder->fld_session, msg_num, &msg);
		if (r != MAIL_NO_ERROR) {
			printf("** message %i not found ** - %s\n", msg_num,
				maildriver_strerror(r));
// 			optind ++;
// 			continue;
		}
printf("asd4\n");
// 		r = mailmessage_get_bodystructure(msg, &mime);
// 		if (r != MAIL_NO_ERROR) {
// 			printf("** message %i not found - %s **\n", msg_num,
// 				maildriver_strerror(r));
// 			mailmessage_free(msg);
// // 			optind ++;
// // 			continue;
// 		}
		
// 		r = etpan_fetch_mime(stdout, msg, mime);

		r = mailmessage_fetch(msg, &data, &size);
		if (r != MAIL_NO_ERROR) {
			printf("** message %i not found - %s **\n", msg_num,
				maildriver_strerror(r));
			mailmessage_free(msg);
// 			optind ++;
// 			continue;
		}
printf("asd5\n");
// fwrite(data, 1, size, stdout);

		mailmessage_free(msg);

// 		optind ++;
// 	}

	mailfolder_free(folder);
	mailstorage_free(storage);
	
	return 0;
}

//  static struct mailimap * imapstorage_get_imap(struct mailstorage * 
// >     storage, mailsession * session)

CRTX_TEST_MAIN(mail_main);
#endif
