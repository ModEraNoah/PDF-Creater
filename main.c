#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define FONTSIZE 12
#define PAGEWIDTH 595
#define PAGELENGTH 842

static uint objectCounter = 0;

typedef struct {
	uint objectNumber;
	char *content;
} Object;

char* getStart() {
	return "%PDF-1.7\n";
}


int getDigitStringWidth(int digit) {
	int charCounter = 1;
	while ((int)(digit/(pow(10, charCounter)))) {
		charCounter++;
	}
	return charCounter;
}

char** craftObjectContent(int objectNumber, char contentString[]) {
	char **res = malloc(sizeof(char*));

	int charCounter = getDigitStringWidth(objectNumber);

	char objectBeginning[charCounter + 8];
	snprintf(objectBeginning, sizeof(objectBeginning), "%i 0 obj\n", objectNumber);

	char *objectEnding = "\nendobj";

	int length = strlen(objectBeginning) + strlen(contentString) + strlen(objectEnding);
	// char *content= (char*) malloc((length + 8) * sizeof(char));
	char *content= (char*) malloc((length+1)  * sizeof(char));
	res[0] = content;

	strcat(content, objectBeginning);
	strcat(content, contentString);
	strcat(content, objectEnding);

	return res;
}

Object getPdfRoot(int objectNumber) {
	char **content = craftObjectContent(objectNumber, "<</Type /Catalog /Pages 2 0 R>>");

	Object temp = {objectNumber, *content};
	return temp;
}

Object getPagesRoot(int objectNumber, int count, int kids[]) {
	char str[200];
	sprintf(str, "<</Type /Pages /Kids [3 0 R] /Count 1 /MediaBox [0 0 %d %d]>>", PAGEWIDTH, PAGELENGTH);
	char **content = craftObjectContent(objectNumber, str);

	Object temp = {objectNumber,*content};
	return temp;
}

Object createPageObject(int objectNumber, int parent, int contents) {

	int additionalLen = 0;
	additionalLen = getDigitStringWidth(parent);
	additionalLen += getDigitStringWidth(contents);

	char *str = "<</Type /Page /Parent %d 0 R /Contents %d 0 R>>";
	int contentLength = strlen(str) + 1;
	char contentString[contentLength + additionalLen];
	snprintf(contentString, contentLength, str, parent, contents);

	char **content = craftObjectContent(objectNumber, contentString);

	Object temp = {objectNumber,*content};
	return temp;
}

char** getFormattedText(char text[]) {
	char **t = (char**) malloc(sizeof(char*));
	t[0] = (char*) malloc(strlen(text) + 100*sizeof(char));

	int charPosition = 1;
	int rowCount = 1;
	int characterCount = 0;
	
	char currentWord[100];
	int currentWordCounter = 0;
	t[0][0] = '(';
	for (int i = 0; text[i] != '\0'; i++) {
		// strcat(t[0], text[i];
		if (text[i] != ' ')
			currentWord[currentWordCounter] = text[i];

		charPosition++;
		characterCount++;
		currentWordCounter++;
		if (text[i] == ' ') {
			// t[0][charPosition-1] = ' ';
			// 0.5 for Time Roman; 0.6 for e.g. Arial
		if ((int)((characterCount) / ((PAGEWIDTH - 15)/(0.5 * FONTSIZE))) == 1) {
				strcat(t[0], ")");
				charPosition++;
				strcat(t[0], " ");
				charPosition++;

				if (rowCount == 1) {
					strcat(t[0], "T");
					charPosition++;
					strcat(t[0], "j");
					charPosition++;
				} else {
					strcat(t[0], "\'");
					charPosition++;
				}
				strcat(t[0], "\n");
				charPosition++;
				strcat(t[0], "(");
				charPosition++;
				strcat(t[0], currentWord);

				characterCount = 0;
				rowCount++;
			} else {
				strcat(t[0], currentWord);
			}
			strcat(t[0], " ");
			memset(currentWord, 0, 100);
			currentWordCounter = 0;
		}
	}

	char isTooLongAtEnd = (int)((characterCount) / ((PAGEWIDTH - 15)/(0.5 * FONTSIZE))) == 0;
	if (isTooLongAtEnd) {
		strcat(t[0], currentWord);
	} 
	strcat(t[0], ") ");
	if (rowCount == 1) {
		strcat(t[0], "Tj");
	} else {
		strcat(t[0], "\'");
	}

	if(! isTooLongAtEnd) {
		strcat(t[0], "\n(");
		strcat(t[0], currentWord);
		strcat(t[0], ") \'");
	}
	
	return t;
}

Object createTextObject(int objectNumber, char text[]) {
		
	char **formattedText = getFormattedText(text);
	int textLen = strlen(formattedText[0]) + 1;
	char *str = "stream\nBT\n/F1 %d Tf\n%d TL\n30 %d Td\n%s\nET\nendstream";
	int strLength = strlen(str) + 1;
	int streamLength = textLen + strLength + 1 + 2*getDigitStringWidth(FONTSIZE) + getDigitStringWidth(PAGELENGTH);// + 12;
	char stream[streamLength];
	snprintf(stream, streamLength, str, FONTSIZE, FONTSIZE, PAGELENGTH - FONTSIZE - 20,formattedText[0]);
	
	int charCounter = getDigitStringWidth(streamLength);
	char *dictStr = "<</Length %d>>\n";
	int dictLength = strlen(dictStr) + 1;
	char dict[charCounter + dictLength] ;
	snprintf(dict, dictLength + charCounter, dictStr, streamLength);

	char contentString[streamLength + charCounter + dictLength + 1];
	strcpy(contentString, dict);
	strcat(contentString,stream);

	char **content = craftObjectContent(objectNumber, contentString);

	Object temp = {objectNumber,*content};
	return temp;
}

static int currentLength = 10;
char** getXref(Object obs[]) {
	char ** t = malloc (1 * sizeof(char*));

	char num[4];
	snprintf(num, sizeof(num), "%d\n", objectCounter+1);
	char temp[6] = "0 ";
	strcat(temp, num);
	t[0] = malloc(1000 * sizeof(char));
	strcat(t[0], "xref\n");
	strcat(t[0], temp);
	strcat(t[0],"0000000000 65535 f\n");
	for (int i = 0; i < objectCounter; i++) {
		char numb[11];
		snprintf(numb, sizeof(numb), "%010d", (currentLength));
		currentLength += (strlen(obs[i].content) + 1);
		strcat(t[0], numb);
		strcat(t[0], " 00000 n\n");
	}
	strcat(t[0], "trailer\n");
	return t;
}

int main(void) {
	Object obs[4];
	obs[0] = getPdfRoot(++objectCounter);
	obs[1] = getPagesRoot(++objectCounter, 1, (int[]){3});
	printf("%s",getStart());

	printf("%s\n", obs[0].content);

	printf("%s\n", obs[1].content);

	int parent = objectCounter;
	obs[2] = createPageObject(++objectCounter, parent, parent+2);
	printf("%s\n", obs[2].content);

	// obs[3]= createTextObject(++objectCounter, "Hallo Welt! Blub... Neue Zeile? hier steht noch viel mehr.... und noch was... sklfaslfdkajwejfkesf ksjfsfej");
	char *textInput = malloc(10000 * sizeof(char));
	int c;
	int i = 0;
    while ((c = getchar()) != EOF) {
		// strcat(textInput, (char*)c);
		textInput[i] = c;
		i++;
    }
	obs[3]= createTextObject(++objectCounter, textInput);
	printf("%s\n", obs[3].content);

	char **temp = getXref(obs);
	printf("%s", temp[0]);
	printf("<</Size %d /Root 1 0 R>>\n", objectCounter+1);
	printf("startxref\n");
	printf("%d\n", currentLength);
	printf("%%%%EOF\n");

	free(temp);

	return 0;
}
