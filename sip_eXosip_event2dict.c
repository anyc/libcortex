/*
 * This file has been generated using:
 *  ./hdr2dict.py --structs eXosip_event /usr/include/eXosip2/eXosip.h --max-recursion 4
 */

#include <string.h>
#include <eXosip2/eXosip.h>

#include "dict.h"

char crtx_eXosip_event2dict(struct eXosip_event *ptr, struct crtx_dict **dict_ptr)
{
	struct crtx_dict *dict;
	struct crtx_dict_item *di;

	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "type_id", ptr->type, sizeof(ptr->type), 0);

	di = crtx_alloc_item(dict);
	switch (ptr->type) {
		case EXOSIP_REGISTRATION_SUCCESS:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_REGISTRATION_SUCCESS", sizeof("EXOSIP_REGISTRATION_SUCCESS"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_REGISTRATION_FAILURE", sizeof("EXOSIP_REGISTRATION_FAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_INVITE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_INVITE", sizeof("EXOSIP_CALL_INVITE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_REINVITE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_REINVITE", sizeof("EXOSIP_CALL_REINVITE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_NOANSWER:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_NOANSWER", sizeof("EXOSIP_CALL_NOANSWER"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_PROCEEDING:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_PROCEEDING", sizeof("EXOSIP_CALL_PROCEEDING"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_RINGING:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_RINGING", sizeof("EXOSIP_CALL_RINGING"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_ANSWERED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_ANSWERED", sizeof("EXOSIP_CALL_ANSWERED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_REDIRECTED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_REDIRECTED", sizeof("EXOSIP_CALL_REDIRECTED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_REQUESTFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_REQUESTFAILURE", sizeof("EXOSIP_CALL_REQUESTFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_SERVERFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_SERVERFAILURE", sizeof("EXOSIP_CALL_SERVERFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_GLOBALFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_GLOBALFAILURE", sizeof("EXOSIP_CALL_GLOBALFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_ACK:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_ACK", sizeof("EXOSIP_CALL_ACK"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_CANCELLED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_CANCELLED", sizeof("EXOSIP_CALL_CANCELLED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_MESSAGE_NEW:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_MESSAGE_NEW", sizeof("EXOSIP_CALL_MESSAGE_NEW"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_MESSAGE_PROCEEDING:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_MESSAGE_PROCEEDING", sizeof("EXOSIP_CALL_MESSAGE_PROCEEDING"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_MESSAGE_ANSWERED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_MESSAGE_ANSWERED", sizeof("EXOSIP_CALL_MESSAGE_ANSWERED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_MESSAGE_REDIRECTED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_MESSAGE_REDIRECTED", sizeof("EXOSIP_CALL_MESSAGE_REDIRECTED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_MESSAGE_REQUESTFAILURE", sizeof("EXOSIP_CALL_MESSAGE_REQUESTFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_MESSAGE_SERVERFAILURE", sizeof("EXOSIP_CALL_MESSAGE_SERVERFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_MESSAGE_GLOBALFAILURE", sizeof("EXOSIP_CALL_MESSAGE_GLOBALFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_CLOSED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_CLOSED", sizeof("EXOSIP_CALL_CLOSED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_CALL_RELEASED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_CALL_RELEASED", sizeof("EXOSIP_CALL_RELEASED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_MESSAGE_NEW:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_MESSAGE_NEW", sizeof("EXOSIP_MESSAGE_NEW"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_MESSAGE_PROCEEDING:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_MESSAGE_PROCEEDING", sizeof("EXOSIP_MESSAGE_PROCEEDING"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_MESSAGE_ANSWERED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_MESSAGE_ANSWERED", sizeof("EXOSIP_MESSAGE_ANSWERED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_MESSAGE_REDIRECTED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_MESSAGE_REDIRECTED", sizeof("EXOSIP_MESSAGE_REDIRECTED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_MESSAGE_REQUESTFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_MESSAGE_REQUESTFAILURE", sizeof("EXOSIP_MESSAGE_REQUESTFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_MESSAGE_SERVERFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_MESSAGE_SERVERFAILURE", sizeof("EXOSIP_MESSAGE_SERVERFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_MESSAGE_GLOBALFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_MESSAGE_GLOBALFAILURE", sizeof("EXOSIP_MESSAGE_GLOBALFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_NOANSWER:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_NOANSWER", sizeof("EXOSIP_SUBSCRIPTION_NOANSWER"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_PROCEEDING:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_PROCEEDING", sizeof("EXOSIP_SUBSCRIPTION_PROCEEDING"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_ANSWERED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_ANSWERED", sizeof("EXOSIP_SUBSCRIPTION_ANSWERED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_REDIRECTED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_REDIRECTED", sizeof("EXOSIP_SUBSCRIPTION_REDIRECTED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_REQUESTFAILURE", sizeof("EXOSIP_SUBSCRIPTION_REQUESTFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_SERVERFAILURE", sizeof("EXOSIP_SUBSCRIPTION_SERVERFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_GLOBALFAILURE", sizeof("EXOSIP_SUBSCRIPTION_GLOBALFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_SUBSCRIPTION_NOTIFY:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_SUBSCRIPTION_NOTIFY", sizeof("EXOSIP_SUBSCRIPTION_NOTIFY"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_IN_SUBSCRIPTION_NEW:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_IN_SUBSCRIPTION_NEW", sizeof("EXOSIP_IN_SUBSCRIPTION_NEW"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_NOTIFICATION_NOANSWER:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_NOTIFICATION_NOANSWER", sizeof("EXOSIP_NOTIFICATION_NOANSWER"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_NOTIFICATION_PROCEEDING:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_NOTIFICATION_PROCEEDING", sizeof("EXOSIP_NOTIFICATION_PROCEEDING"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_NOTIFICATION_ANSWERED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_NOTIFICATION_ANSWERED", sizeof("EXOSIP_NOTIFICATION_ANSWERED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_NOTIFICATION_REDIRECTED:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_NOTIFICATION_REDIRECTED", sizeof("EXOSIP_NOTIFICATION_REDIRECTED"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_NOTIFICATION_REQUESTFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_NOTIFICATION_REQUESTFAILURE", sizeof("EXOSIP_NOTIFICATION_REQUESTFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_NOTIFICATION_SERVERFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_NOTIFICATION_SERVERFAILURE", sizeof("EXOSIP_NOTIFICATION_SERVERFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_NOTIFICATION_GLOBALFAILURE:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_NOTIFICATION_GLOBALFAILURE", sizeof("EXOSIP_NOTIFICATION_GLOBALFAILURE"), CRTX_DIF_DONT_FREE_DATA);
			break;
		case EXOSIP_EVENT_COUNT:
			crtx_fill_data_item(di, 's', "type", "EXOSIP_EVENT_COUNT", sizeof("EXOSIP_EVENT_COUNT"), CRTX_DIF_DONT_FREE_DATA);
			break;
	}

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "textinfo", ptr->textinfo, sizeof(ptr->textinfo), CRTX_DIF_CREATE_DATA_COPY);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'p', "external_reference", ptr->external_reference, 0, CRTX_DIF_DONT_FREE_DATA);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "request", 0, 0, 0);
	if (ptr->request) {
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;

		if (ptr->request->sip_version) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "sip_version", ptr->request->sip_version, strlen(ptr->request->sip_version), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "req_uri", 0, 0, 0);
		if (ptr->request->req_uri) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->req_uri->scheme) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "scheme", ptr->request->req_uri->scheme, strlen(ptr->request->req_uri->scheme), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->request->req_uri->username) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "username", ptr->request->req_uri->username, strlen(ptr->request->req_uri->username), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->request->req_uri->password) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "password", ptr->request->req_uri->password, strlen(ptr->request->req_uri->password), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->request->req_uri->host) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "host", ptr->request->req_uri->host, strlen(ptr->request->req_uri->host), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->request->req_uri->port) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "port", ptr->request->req_uri->port, strlen(ptr->request->req_uri->port), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->request->req_uri->url_params.nb_elt, sizeof(ptr->request->req_uri->url_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->request->req_uri->url_params.node, di2->dict);

			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url_headers", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->request->req_uri->url_headers.nb_elt, sizeof(ptr->request->req_uri->url_headers.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->request->req_uri->url_headers.node, di2->dict);

			}

			if (ptr->request->req_uri->string) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "string", ptr->request->req_uri->string, strlen(ptr->request->req_uri->string), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		if (ptr->request->sip_method) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "sip_method", ptr->request->sip_method, strlen(ptr->request->sip_method), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "status_code", ptr->request->status_code, sizeof(ptr->request->status_code), 0);

		if (ptr->request->reason_phrase) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "reason_phrase", ptr->request->reason_phrase, strlen(ptr->request->reason_phrase), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accepts", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->accepts.nb_elt, sizeof(ptr->request->accepts.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->accepts.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->accepts.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->accepts.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accept_encodings", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->accept_encodings.nb_elt, sizeof(ptr->request->accept_encodings.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->accept_encodings.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->accept_encodings.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->accept_encodings.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accept_languages", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->accept_languages.nb_elt, sizeof(ptr->request->accept_languages.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->accept_languages.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->accept_languages.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->accept_languages.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "alert_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->alert_infos.nb_elt, sizeof(ptr->request->alert_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->alert_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->alert_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->alert_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "allows", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->allows.nb_elt, sizeof(ptr->request->allows.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->allows.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->allows.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->allows.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "authentication_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->authentication_infos.nb_elt, sizeof(ptr->request->authentication_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->authentication_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->authentication_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->authentication_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "authorizations", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->authorizations.nb_elt, sizeof(ptr->request->authorizations.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->authorizations.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->authorizations.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->authorizations.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "call_id", 0, 0, 0);
		if (ptr->request->call_id) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->call_id->number) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "number", ptr->request->call_id->number, strlen(ptr->request->call_id->number), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->request->call_id->host) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "host", ptr->request->call_id->host, strlen(ptr->request->call_id->host), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "call_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->call_infos.nb_elt, sizeof(ptr->request->call_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->call_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->call_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->call_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "contacts", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->contacts.nb_elt, sizeof(ptr->request->contacts.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->contacts.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->contacts.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->contacts.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_encodings", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->content_encodings.nb_elt, sizeof(ptr->request->content_encodings.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->content_encodings.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->content_encodings.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->content_encodings.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_length", 0, 0, 0);
		if (ptr->request->content_length) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->content_length->value) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "value", ptr->request->content_length->value, strlen(ptr->request->content_length->value), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_type", 0, 0, 0);
		if (ptr->request->content_type) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->content_type->type) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "type", ptr->request->content_type->type, strlen(ptr->request->content_type->type), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->request->content_type->subtype) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "subtype", ptr->request->content_type->subtype, strlen(ptr->request->content_type->subtype), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->request->content_type->gen_params.nb_elt, sizeof(ptr->request->content_type->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->request->content_type->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "cseq", 0, 0, 0);
		if (ptr->request->cseq) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->cseq->method) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "method", ptr->request->cseq->method, strlen(ptr->request->cseq->method), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->request->cseq->number) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "number", ptr->request->cseq->number, strlen(ptr->request->cseq->number), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "error_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->error_infos.nb_elt, sizeof(ptr->request->error_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->error_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->error_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->error_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "from", 0, 0, 0);
		if (ptr->request->from) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->from->displayname) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "displayname", ptr->request->from->displayname, strlen(ptr->request->from->displayname), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url", 0, 0, 0);
			if (ptr->request->from->url) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				if (ptr->request->from->url->scheme) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "scheme", ptr->request->from->url->scheme, strlen(ptr->request->from->url->scheme), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->from->url->username) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "username", ptr->request->from->url->username, strlen(ptr->request->from->url->username), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->from->url->password) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "password", ptr->request->from->url->password, strlen(ptr->request->from->url->password), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->from->url->host) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "host", ptr->request->from->url->host, strlen(ptr->request->from->url->host), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->from->url->port) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "port", ptr->request->from->url->port, strlen(ptr->request->from->url->port), CRTX_DIF_DONT_FREE_DATA);
				}

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_params", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->request->from->url->url_params, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_headers", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->request->from->url->url_headers, di2->dict);


				if (ptr->request->from->url->string) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "string", ptr->request->from->url->string, strlen(ptr->request->from->url->string), CRTX_DIF_DONT_FREE_DATA);
				}
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->request->from->gen_params.nb_elt, sizeof(ptr->request->from->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->request->from->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "mime_version", 0, 0, 0);
		if (ptr->request->mime_version) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->mime_version->value) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "value", ptr->request->mime_version->value, strlen(ptr->request->mime_version->value), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authenticates", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->proxy_authenticates.nb_elt, sizeof(ptr->request->proxy_authenticates.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->proxy_authenticates.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->proxy_authenticates.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->proxy_authenticates.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authentication_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->proxy_authentication_infos.nb_elt, sizeof(ptr->request->proxy_authentication_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->proxy_authentication_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->proxy_authentication_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->proxy_authentication_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authorizations", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->proxy_authorizations.nb_elt, sizeof(ptr->request->proxy_authorizations.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->proxy_authorizations.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->proxy_authorizations.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->proxy_authorizations.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "record_routes", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->record_routes.nb_elt, sizeof(ptr->request->record_routes.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->record_routes.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->record_routes.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->record_routes.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "routes", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->routes.nb_elt, sizeof(ptr->request->routes.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->routes.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->routes.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->routes.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "to", 0, 0, 0);
		if (ptr->request->to) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->request->to->displayname) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "displayname", ptr->request->to->displayname, strlen(ptr->request->to->displayname), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url", 0, 0, 0);
			if (ptr->request->to->url) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				if (ptr->request->to->url->scheme) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "scheme", ptr->request->to->url->scheme, strlen(ptr->request->to->url->scheme), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->to->url->username) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "username", ptr->request->to->url->username, strlen(ptr->request->to->url->username), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->to->url->password) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "password", ptr->request->to->url->password, strlen(ptr->request->to->url->password), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->to->url->host) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "host", ptr->request->to->url->host, strlen(ptr->request->to->url->host), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->request->to->url->port) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "port", ptr->request->to->url->port, strlen(ptr->request->to->url->port), CRTX_DIF_DONT_FREE_DATA);
				}

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_params", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->request->to->url->url_params, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_headers", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->request->to->url->url_headers, di2->dict);


				if (ptr->request->to->url->string) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "string", ptr->request->to->url->string, strlen(ptr->request->to->url->string), CRTX_DIF_DONT_FREE_DATA);
				}
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->request->to->gen_params.nb_elt, sizeof(ptr->request->to->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->request->to->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "vias", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->vias.nb_elt, sizeof(ptr->request->vias.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->vias.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->vias.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->vias.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "www_authenticates", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->www_authenticates.nb_elt, sizeof(ptr->request->www_authenticates.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->www_authenticates.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->www_authenticates.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->www_authenticates.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "headers", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->headers.nb_elt, sizeof(ptr->request->headers.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->headers.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->headers.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->headers.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "bodies", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->request->bodies.nb_elt, sizeof(ptr->request->bodies.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->request->bodies.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->request->bodies.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->request->bodies.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "message_property", ptr->request->message_property, sizeof(ptr->request->message_property), 0);

		if (ptr->request->message) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "message", ptr->request->message, strlen(ptr->request->message), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'z', "message_length", ptr->request->message_length, sizeof(ptr->request->message_length), 0);

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'p', "application_data", ptr->request->application_data, 0, CRTX_DIF_DONT_FREE_DATA);
	}

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "response", 0, 0, 0);
	if (ptr->response) {
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;

		if (ptr->response->sip_version) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "sip_version", ptr->response->sip_version, strlen(ptr->response->sip_version), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "req_uri", 0, 0, 0);
		if (ptr->response->req_uri) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->req_uri->scheme) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "scheme", ptr->response->req_uri->scheme, strlen(ptr->response->req_uri->scheme), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->response->req_uri->username) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "username", ptr->response->req_uri->username, strlen(ptr->response->req_uri->username), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->response->req_uri->password) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "password", ptr->response->req_uri->password, strlen(ptr->response->req_uri->password), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->response->req_uri->host) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "host", ptr->response->req_uri->host, strlen(ptr->response->req_uri->host), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->response->req_uri->port) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "port", ptr->response->req_uri->port, strlen(ptr->response->req_uri->port), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->response->req_uri->url_params.nb_elt, sizeof(ptr->response->req_uri->url_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->response->req_uri->url_params.node, di2->dict);

			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url_headers", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->response->req_uri->url_headers.nb_elt, sizeof(ptr->response->req_uri->url_headers.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->response->req_uri->url_headers.node, di2->dict);

			}

			if (ptr->response->req_uri->string) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "string", ptr->response->req_uri->string, strlen(ptr->response->req_uri->string), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		if (ptr->response->sip_method) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "sip_method", ptr->response->sip_method, strlen(ptr->response->sip_method), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "status_code", ptr->response->status_code, sizeof(ptr->response->status_code), 0);

		if (ptr->response->reason_phrase) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "reason_phrase", ptr->response->reason_phrase, strlen(ptr->response->reason_phrase), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accepts", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->accepts.nb_elt, sizeof(ptr->response->accepts.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->accepts.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->accepts.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->accepts.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accept_encodings", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->accept_encodings.nb_elt, sizeof(ptr->response->accept_encodings.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->accept_encodings.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->accept_encodings.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->accept_encodings.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accept_languages", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->accept_languages.nb_elt, sizeof(ptr->response->accept_languages.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->accept_languages.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->accept_languages.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->accept_languages.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "alert_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->alert_infos.nb_elt, sizeof(ptr->response->alert_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->alert_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->alert_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->alert_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "allows", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->allows.nb_elt, sizeof(ptr->response->allows.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->allows.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->allows.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->allows.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "authentication_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->authentication_infos.nb_elt, sizeof(ptr->response->authentication_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->authentication_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->authentication_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->authentication_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "authorizations", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->authorizations.nb_elt, sizeof(ptr->response->authorizations.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->authorizations.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->authorizations.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->authorizations.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "call_id", 0, 0, 0);
		if (ptr->response->call_id) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->call_id->number) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "number", ptr->response->call_id->number, strlen(ptr->response->call_id->number), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->response->call_id->host) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "host", ptr->response->call_id->host, strlen(ptr->response->call_id->host), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "call_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->call_infos.nb_elt, sizeof(ptr->response->call_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->call_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->call_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->call_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "contacts", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->contacts.nb_elt, sizeof(ptr->response->contacts.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->contacts.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->contacts.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->contacts.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_encodings", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->content_encodings.nb_elt, sizeof(ptr->response->content_encodings.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->content_encodings.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->content_encodings.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->content_encodings.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_length", 0, 0, 0);
		if (ptr->response->content_length) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->content_length->value) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "value", ptr->response->content_length->value, strlen(ptr->response->content_length->value), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_type", 0, 0, 0);
		if (ptr->response->content_type) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->content_type->type) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "type", ptr->response->content_type->type, strlen(ptr->response->content_type->type), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->response->content_type->subtype) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "subtype", ptr->response->content_type->subtype, strlen(ptr->response->content_type->subtype), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->response->content_type->gen_params.nb_elt, sizeof(ptr->response->content_type->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->response->content_type->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "cseq", 0, 0, 0);
		if (ptr->response->cseq) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->cseq->method) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "method", ptr->response->cseq->method, strlen(ptr->response->cseq->method), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->response->cseq->number) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "number", ptr->response->cseq->number, strlen(ptr->response->cseq->number), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "error_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->error_infos.nb_elt, sizeof(ptr->response->error_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->error_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->error_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->error_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "from", 0, 0, 0);
		if (ptr->response->from) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->from->displayname) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "displayname", ptr->response->from->displayname, strlen(ptr->response->from->displayname), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url", 0, 0, 0);
			if (ptr->response->from->url) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				if (ptr->response->from->url->scheme) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "scheme", ptr->response->from->url->scheme, strlen(ptr->response->from->url->scheme), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->from->url->username) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "username", ptr->response->from->url->username, strlen(ptr->response->from->url->username), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->from->url->password) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "password", ptr->response->from->url->password, strlen(ptr->response->from->url->password), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->from->url->host) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "host", ptr->response->from->url->host, strlen(ptr->response->from->url->host), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->from->url->port) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "port", ptr->response->from->url->port, strlen(ptr->response->from->url->port), CRTX_DIF_DONT_FREE_DATA);
				}

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_params", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->response->from->url->url_params, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_headers", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->response->from->url->url_headers, di2->dict);


				if (ptr->response->from->url->string) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "string", ptr->response->from->url->string, strlen(ptr->response->from->url->string), CRTX_DIF_DONT_FREE_DATA);
				}
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->response->from->gen_params.nb_elt, sizeof(ptr->response->from->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->response->from->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "mime_version", 0, 0, 0);
		if (ptr->response->mime_version) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->mime_version->value) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "value", ptr->response->mime_version->value, strlen(ptr->response->mime_version->value), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authenticates", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->proxy_authenticates.nb_elt, sizeof(ptr->response->proxy_authenticates.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->proxy_authenticates.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->proxy_authenticates.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->proxy_authenticates.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authentication_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->proxy_authentication_infos.nb_elt, sizeof(ptr->response->proxy_authentication_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->proxy_authentication_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->proxy_authentication_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->proxy_authentication_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authorizations", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->proxy_authorizations.nb_elt, sizeof(ptr->response->proxy_authorizations.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->proxy_authorizations.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->proxy_authorizations.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->proxy_authorizations.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "record_routes", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->record_routes.nb_elt, sizeof(ptr->response->record_routes.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->record_routes.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->record_routes.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->record_routes.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "routes", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->routes.nb_elt, sizeof(ptr->response->routes.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->routes.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->routes.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->routes.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "to", 0, 0, 0);
		if (ptr->response->to) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->response->to->displayname) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "displayname", ptr->response->to->displayname, strlen(ptr->response->to->displayname), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url", 0, 0, 0);
			if (ptr->response->to->url) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				if (ptr->response->to->url->scheme) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "scheme", ptr->response->to->url->scheme, strlen(ptr->response->to->url->scheme), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->to->url->username) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "username", ptr->response->to->url->username, strlen(ptr->response->to->url->username), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->to->url->password) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "password", ptr->response->to->url->password, strlen(ptr->response->to->url->password), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->to->url->host) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "host", ptr->response->to->url->host, strlen(ptr->response->to->url->host), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->response->to->url->port) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "port", ptr->response->to->url->port, strlen(ptr->response->to->url->port), CRTX_DIF_DONT_FREE_DATA);
				}

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_params", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->response->to->url->url_params, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_headers", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->response->to->url->url_headers, di2->dict);


				if (ptr->response->to->url->string) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "string", ptr->response->to->url->string, strlen(ptr->response->to->url->string), CRTX_DIF_DONT_FREE_DATA);
				}
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->response->to->gen_params.nb_elt, sizeof(ptr->response->to->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->response->to->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "vias", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->vias.nb_elt, sizeof(ptr->response->vias.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->vias.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->vias.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->vias.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "www_authenticates", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->www_authenticates.nb_elt, sizeof(ptr->response->www_authenticates.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->www_authenticates.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->www_authenticates.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->www_authenticates.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "headers", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->headers.nb_elt, sizeof(ptr->response->headers.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->headers.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->headers.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->headers.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "bodies", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->response->bodies.nb_elt, sizeof(ptr->response->bodies.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->response->bodies.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->response->bodies.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->response->bodies.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "message_property", ptr->response->message_property, sizeof(ptr->response->message_property), 0);

		if (ptr->response->message) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "message", ptr->response->message, strlen(ptr->response->message), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'z', "message_length", ptr->response->message_length, sizeof(ptr->response->message_length), 0);

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'p', "application_data", ptr->response->application_data, 0, CRTX_DIF_DONT_FREE_DATA);
	}

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "ack", 0, 0, 0);
	if (ptr->ack) {
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;

		if (ptr->ack->sip_version) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "sip_version", ptr->ack->sip_version, strlen(ptr->ack->sip_version), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "req_uri", 0, 0, 0);
		if (ptr->ack->req_uri) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->req_uri->scheme) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "scheme", ptr->ack->req_uri->scheme, strlen(ptr->ack->req_uri->scheme), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->ack->req_uri->username) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "username", ptr->ack->req_uri->username, strlen(ptr->ack->req_uri->username), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->ack->req_uri->password) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "password", ptr->ack->req_uri->password, strlen(ptr->ack->req_uri->password), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->ack->req_uri->host) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "host", ptr->ack->req_uri->host, strlen(ptr->ack->req_uri->host), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->ack->req_uri->port) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "port", ptr->ack->req_uri->port, strlen(ptr->ack->req_uri->port), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->ack->req_uri->url_params.nb_elt, sizeof(ptr->ack->req_uri->url_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->ack->req_uri->url_params.node, di2->dict);

			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url_headers", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->ack->req_uri->url_headers.nb_elt, sizeof(ptr->ack->req_uri->url_headers.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->ack->req_uri->url_headers.node, di2->dict);

			}

			if (ptr->ack->req_uri->string) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "string", ptr->ack->req_uri->string, strlen(ptr->ack->req_uri->string), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		if (ptr->ack->sip_method) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "sip_method", ptr->ack->sip_method, strlen(ptr->ack->sip_method), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "status_code", ptr->ack->status_code, sizeof(ptr->ack->status_code), 0);

		if (ptr->ack->reason_phrase) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "reason_phrase", ptr->ack->reason_phrase, strlen(ptr->ack->reason_phrase), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accepts", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->accepts.nb_elt, sizeof(ptr->ack->accepts.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->accepts.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->accepts.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->accepts.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accept_encodings", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->accept_encodings.nb_elt, sizeof(ptr->ack->accept_encodings.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->accept_encodings.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->accept_encodings.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->accept_encodings.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "accept_languages", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->accept_languages.nb_elt, sizeof(ptr->ack->accept_languages.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->accept_languages.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->accept_languages.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->accept_languages.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "alert_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->alert_infos.nb_elt, sizeof(ptr->ack->alert_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->alert_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->alert_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->alert_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "allows", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->allows.nb_elt, sizeof(ptr->ack->allows.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->allows.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->allows.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->allows.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "authentication_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->authentication_infos.nb_elt, sizeof(ptr->ack->authentication_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->authentication_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->authentication_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->authentication_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "authorizations", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->authorizations.nb_elt, sizeof(ptr->ack->authorizations.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->authorizations.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->authorizations.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->authorizations.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "call_id", 0, 0, 0);
		if (ptr->ack->call_id) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->call_id->number) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "number", ptr->ack->call_id->number, strlen(ptr->ack->call_id->number), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->ack->call_id->host) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "host", ptr->ack->call_id->host, strlen(ptr->ack->call_id->host), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "call_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->call_infos.nb_elt, sizeof(ptr->ack->call_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->call_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->call_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->call_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "contacts", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->contacts.nb_elt, sizeof(ptr->ack->contacts.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->contacts.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->contacts.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->contacts.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_encodings", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->content_encodings.nb_elt, sizeof(ptr->ack->content_encodings.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->content_encodings.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->content_encodings.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->content_encodings.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_length", 0, 0, 0);
		if (ptr->ack->content_length) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->content_length->value) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "value", ptr->ack->content_length->value, strlen(ptr->ack->content_length->value), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "content_type", 0, 0, 0);
		if (ptr->ack->content_type) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->content_type->type) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "type", ptr->ack->content_type->type, strlen(ptr->ack->content_type->type), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->ack->content_type->subtype) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "subtype", ptr->ack->content_type->subtype, strlen(ptr->ack->content_type->subtype), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->ack->content_type->gen_params.nb_elt, sizeof(ptr->ack->content_type->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->ack->content_type->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "cseq", 0, 0, 0);
		if (ptr->ack->cseq) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->cseq->method) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "method", ptr->ack->cseq->method, strlen(ptr->ack->cseq->method), CRTX_DIF_DONT_FREE_DATA);
			}

			if (ptr->ack->cseq->number) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "number", ptr->ack->cseq->number, strlen(ptr->ack->cseq->number), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "error_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->error_infos.nb_elt, sizeof(ptr->ack->error_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->error_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->error_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->error_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "from", 0, 0, 0);
		if (ptr->ack->from) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->from->displayname) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "displayname", ptr->ack->from->displayname, strlen(ptr->ack->from->displayname), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url", 0, 0, 0);
			if (ptr->ack->from->url) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				if (ptr->ack->from->url->scheme) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "scheme", ptr->ack->from->url->scheme, strlen(ptr->ack->from->url->scheme), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->from->url->username) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "username", ptr->ack->from->url->username, strlen(ptr->ack->from->url->username), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->from->url->password) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "password", ptr->ack->from->url->password, strlen(ptr->ack->from->url->password), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->from->url->host) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "host", ptr->ack->from->url->host, strlen(ptr->ack->from->url->host), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->from->url->port) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "port", ptr->ack->from->url->port, strlen(ptr->ack->from->url->port), CRTX_DIF_DONT_FREE_DATA);
				}

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_params", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->ack->from->url->url_params, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_headers", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->ack->from->url->url_headers, di2->dict);


				if (ptr->ack->from->url->string) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "string", ptr->ack->from->url->string, strlen(ptr->ack->from->url->string), CRTX_DIF_DONT_FREE_DATA);
				}
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->ack->from->gen_params.nb_elt, sizeof(ptr->ack->from->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->ack->from->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "mime_version", 0, 0, 0);
		if (ptr->ack->mime_version) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->mime_version->value) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "value", ptr->ack->mime_version->value, strlen(ptr->ack->mime_version->value), CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authenticates", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->proxy_authenticates.nb_elt, sizeof(ptr->ack->proxy_authenticates.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->proxy_authenticates.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->proxy_authenticates.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->proxy_authenticates.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authentication_infos", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->proxy_authentication_infos.nb_elt, sizeof(ptr->ack->proxy_authentication_infos.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->proxy_authentication_infos.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->proxy_authentication_infos.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->proxy_authentication_infos.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "proxy_authorizations", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->proxy_authorizations.nb_elt, sizeof(ptr->ack->proxy_authorizations.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->proxy_authorizations.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->proxy_authorizations.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->proxy_authorizations.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "record_routes", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->record_routes.nb_elt, sizeof(ptr->ack->record_routes.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->record_routes.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->record_routes.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->record_routes.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "routes", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->routes.nb_elt, sizeof(ptr->ack->routes.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->routes.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->routes.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->routes.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "to", 0, 0, 0);
		if (ptr->ack->to) {
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			if (ptr->ack->to->displayname) {
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "displayname", ptr->ack->to->displayname, strlen(ptr->ack->to->displayname), CRTX_DIF_DONT_FREE_DATA);
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "url", 0, 0, 0);
			if (ptr->ack->to->url) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				if (ptr->ack->to->url->scheme) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "scheme", ptr->ack->to->url->scheme, strlen(ptr->ack->to->url->scheme), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->to->url->username) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "username", ptr->ack->to->url->username, strlen(ptr->ack->to->url->username), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->to->url->password) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "password", ptr->ack->to->url->password, strlen(ptr->ack->to->url->password), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->to->url->host) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "host", ptr->ack->to->url->host, strlen(ptr->ack->to->url->host), CRTX_DIF_DONT_FREE_DATA);
				}

				if (ptr->ack->to->url->port) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "port", ptr->ack->to->url->port, strlen(ptr->ack->to->url->port), CRTX_DIF_DONT_FREE_DATA);
				}

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_params", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->ack->to->url->url_params, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "url_headers", 0, 0, 0);
				// crtx_osip_list2dict(&ptr->ack->to->url->url_headers, di2->dict);


				if (ptr->ack->to->url->string) {
					di2 = crtx_alloc_item(dict);
					crtx_fill_data_item(di2, 's', "string", ptr->ack->to->url->string, strlen(ptr->ack->to->url->string), CRTX_DIF_DONT_FREE_DATA);
				}
			}

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "gen_params", 0, 0, 0);

			{
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'i', "nb_elt", ptr->ack->to->gen_params.nb_elt, sizeof(ptr->ack->to->gen_params.nb_elt), 0);

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "node", 0, 0, 0);
				// crtx___node2dict(ptr->ack->to->gen_params.node, di2->dict);

			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "vias", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->vias.nb_elt, sizeof(ptr->ack->vias.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->vias.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->vias.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->vias.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "www_authenticates", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->www_authenticates.nb_elt, sizeof(ptr->ack->www_authenticates.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->www_authenticates.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->www_authenticates.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->www_authenticates.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "headers", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->headers.nb_elt, sizeof(ptr->ack->headers.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->headers.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->headers.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->headers.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "bodies", 0, 0, 0);

		{
			di2->dict = crtx_init_dict(0, 0, 0);
			struct crtx_dict *dict = di2->dict;
			struct crtx_dict_item *di;

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'i', "nb_elt", ptr->ack->bodies.nb_elt, sizeof(ptr->ack->bodies.nb_elt), 0);

			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 'D', "node", 0, 0, 0);
			if (ptr->ack->bodies.node) {
				di->dict = crtx_init_dict(0, 0, 0);
				struct crtx_dict *dict = di->dict;
				struct crtx_dict_item *di2;

				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'D', "next", 0, 0, 0);
				// crtx___node2dict(ptr->ack->bodies.node->next, di2->dict);


				di2 = crtx_alloc_item(dict);
				crtx_fill_data_item(di2, 'p', "element", ptr->ack->bodies.node->element, 0, CRTX_DIF_DONT_FREE_DATA);
			}
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "message_property", ptr->ack->message_property, sizeof(ptr->ack->message_property), 0);

		if (ptr->ack->message) {
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 's', "message", ptr->ack->message, strlen(ptr->ack->message), CRTX_DIF_DONT_FREE_DATA);
		}

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'z', "message_length", ptr->ack->message_length, sizeof(ptr->ack->message_length), 0);

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'p', "application_data", ptr->ack->application_data, 0, CRTX_DIF_DONT_FREE_DATA);
	}

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "tid", ptr->tid, sizeof(ptr->tid), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "did", ptr->did, sizeof(ptr->did), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "rid", ptr->rid, sizeof(ptr->rid), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "cid", ptr->cid, sizeof(ptr->cid), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "sid", ptr->sid, sizeof(ptr->sid), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "nid", ptr->nid, sizeof(ptr->nid), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "ss_status", ptr->ss_status, sizeof(ptr->ss_status), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "ss_reason", ptr->ss_reason, sizeof(ptr->ss_reason), 0);
	
	return 0;
}



