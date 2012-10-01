// shinglsAlg.cpp: ���������� ����� ����� ��� ����������� ����������.
//
#include "../headers/stdafx.h"
#include "../src/gsoap_autogenerated/Shingle.nsmap"
#include "../headers/targetver.h"
#include "../headers/shingleApp.h"

int http_get(struct soap *soap) 
{ 
   FILE *fd = NULL;
   char *s = strchr(soap->path, '?');
   if (!s || strcmp(s, "?wsdl"))
      return SOAP_GET_METHOD;
   fd = fopen("shingle.wsdl", "rb"); // open WSDL file to copy
   if (!fd)
      return 404; // return HTTP not found error
   soap->http_content = "text/xml"; // HTTP header with text/xml content
   soap_response(soap, SOAP_FILE);
   for (;;)
   {
      size_t r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);
      if (!r)
         break;
      if (soap_send_raw(soap, soap->tmpbuf, r))
         break; // can't send, but little we can do about that
   }
   fclose(fd);
   soap_end_send(soap);
   return SOAP_OK;
}


int _tmain(int argc, _TCHAR* argv[])
{
	const int SERVICE_PORT = 9999;
	setlocale(LC_ALL, "ru_RU.UTF-8");
    std::auto_ptr<ShingleApp> srv (new ShingleApp());
	soap_set_imode(srv, SOAP_C_UTFSTRING);
	soap_set_omode(srv, SOAP_C_UTFSTRING);
	srv->fget = http_get;
	srv->log() << "Server putted up!\n" << srv->nowToStr() << '\n';

    if (srv->run(SERVICE_PORT)){
		srv->log() << "Warning! Server is down at " << srv->nowToStr() << '\n';
	};
	system("pause");
    return 0;
}

