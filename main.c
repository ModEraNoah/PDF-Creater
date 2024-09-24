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

Object getPdfRoot(int objectNumber, int pagesObjectNumber) {
	char *tmp = "<</Type /Catalog /Pages %d 0 R>>";
	int len = strlen(tmp) + getDigitStringWidth(pagesObjectNumber) + 1;
	char buf[len];
	snprintf(buf, len, "<</Type /Catalog /Pages %d 0 R>>", pagesObjectNumber);
	char **content = craftObjectContent(objectNumber, buf);

	Object temp = {objectNumber, *content};
	return temp;
}

Object getPagesRoot(int objectNumber, int count, int kids[]) {
	char kidsArray[200];
	memset(kidsArray,0,200);
	for (int i = 0; i < count; i++) {
		char tmp[20];
		snprintf(tmp, 20, "%d 0 R ", kids[i]);
		strcat(kidsArray, tmp);
	}
	char str[200];
	sprintf(str, "<</Type /Pages /Kids [%s] /Count 1 /MediaBox [0 0 %d %d]>>",kidsArray, PAGEWIDTH, PAGELENGTH);
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
	t[0] = (char*) malloc(strlen(text) + 300*sizeof(char));

	strcpy(t[0], "");
	int rowCount = 1;
	int characterCount = 0;
	
	char currentWord[100];
	int currentWordCounter = 0;
	memset(currentWord, 0, 100);
	t[0][0] = '(';
	for (int i = 0; text[i] != '\0'; i++) {
		if (text[i] != ' ')
			currentWord[currentWordCounter] = text[i];

		characterCount++;
		currentWordCounter++;
		if (text[i] == ' ') {
			// 0.5 for Time Roman; 0.6 for e.g. Arial
			char isLongerThanLine = ((PAGEWIDTH - (5.2* FONTSIZE))/(0.5 * FONTSIZE * characterCount)) < 0.91;
			if (isLongerThanLine) {
				strcat(t[0], ") ");

				if (rowCount == 1) {
					strcat(t[0], "Tj");
				} else {
					strcat(t[0], "\'");
				}
				strcat(t[0], "\n(");
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

	char isTooLongAtEnd = ((PAGEWIDTH - (5.2* FONTSIZE))/(0.5 * FONTSIZE * characterCount)) < 0.91	;
	if (!isTooLongAtEnd) {
		strcat(t[0], currentWord);
	} 
	strcat(t[0], ") ");
	if (rowCount == 1) {
		strcat(t[0], "Tj");
	} else {
		strcat(t[0], "\'");
	}

	if(isTooLongAtEnd) {
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
	char *textInput = malloc(10000 * sizeof(char));
	int c;
	int i = 0;
	while ((c = getchar()) != EOF) {
		textInput[i] = c;
		i++;
	}

	Object obs[4];
	obs[0] = getPagesRoot(++objectCounter, 1, (int[]){4});
	int pagesRootNumber = objectCounter;
	obs[1] = getPdfRoot(++objectCounter, pagesRootNumber);
	Object *root = &obs[1];

	obs[2]= createTextObject(++objectCounter, textInput);
	Object *textObject = &obs[2];

	obs[3] = createPageObject(++objectCounter, pagesRootNumber, textObject->objectNumber);

	printf("%s",getStart());

	printf("%s\n", obs[0].content);

	printf("%s\n", obs[1].content);


	printf("%s\n", obs[2].content);

	printf("%s\n", obs[3].content);

	char **temp = getXref(obs);
	printf("%s", temp[0]);
	// printf("<</Size %d /Root 1 0 R>>\n", objectCounter+1);
	int bufLength = strlen("<</Size %d /Root %d 0 R>>\n") + getDigitStringWidth(objectCounter + 1)+1;
	char buf[bufLength];
	snprintf(buf, bufLength, "<</Size %d /Root %d 0 R>>\n", objectCounter+1, root->objectNumber);
	printf("%s",buf);
	printf("startxref\n");
	printf("%d\n", currentLength);
	printf("%%%%EOF\n");

	free(temp);

	return 0;
}
