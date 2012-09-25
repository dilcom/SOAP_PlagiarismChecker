#pragma once
#include <string>
#include "../src/gsoap_autogenerated/soapStub.h"
#include <time.h>

using namespace std;

namespace DePlaguarism{

	struct TextDocument
	{
		string name,
			authorName,
			authorGroup,
			data;
		t__type type;
		int number;
		tm dateTime;

		TextDocument(const t__text & m, int num);
		TextDocument(){};
	};

	//TextDocument * getShingleAppText(const t__text & m);
}

