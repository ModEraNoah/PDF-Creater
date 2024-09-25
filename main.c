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
	sprintf(str, "<</Type /Pages /Kids [%s] /Count %d /MediaBox [0 0 %d %d]>>",kidsArray, count,PAGEWIDTH, PAGELENGTH);
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

int createFormattedText(char** buf, char inputText[]) {
	const int LENGTH = 4096;
	int pageIndex = 0;

	buf[pageIndex] = malloc(LENGTH);
	strcpy(buf[pageIndex], "");
	int rowCount = 1;
	int characterCount = 0;
	
	char currentWord[100];
	int currentWordCounter = 0;
	strcpy(currentWord, "");
	buf[pageIndex][0] = '(';
	for (int i = 0; inputText[i] != '\0'; i++) {
		if (inputText[i] != ' ')
			currentWord[currentWordCounter] = inputText[i];

		characterCount++;
		currentWordCounter++;
		if (inputText[i] == ' ') {
			// 0.5 for Time Roman; 0.6 for e.g. Arial
			char isLongerThanLine = ((PAGEWIDTH - (5.2* FONTSIZE))/(0.5 * FONTSIZE * characterCount)) < 0.91;
			if (isLongerThanLine) {
				strcat(buf[pageIndex], ") ");

				if (rowCount == 1) {
					strcat(buf[pageIndex], "Tj");
				} else {
					strcat(buf[pageIndex], "\'");
				}

				characterCount = 0;
				rowCount++;
				if (rowCount > 1) {
					pageIndex++;
					rowCount = 0;
					buf[pageIndex] = malloc(LENGTH);
				}

				strcat(buf[pageIndex], "\n(");
				strcat(buf[pageIndex], currentWord);

			} else {
				strcat(buf[pageIndex], currentWord);
			}
			strcat(buf[pageIndex], " ");
			memset(currentWord, 0, 100);
			currentWordCounter = 0;
		}
	}

	char isTooLongAtEnd = ((PAGEWIDTH - (5.2* FONTSIZE))/(0.5 * FONTSIZE * characterCount)) < 0.91	;
	if (!isTooLongAtEnd) {
		strcat(buf[pageIndex], currentWord);
	} 
	strcat(buf[pageIndex], ") ");
	if (rowCount == 1) {
		strcat(buf[pageIndex], "Tj");
	} else {
		strcat(buf[pageIndex], "\'");
	}

	if(isTooLongAtEnd) {
		strcat(buf[pageIndex], "\n(");
		strcat(buf[pageIndex], currentWord);
		strcat(buf[pageIndex], ") \'");
	}
	
	/* +1 as the index starts at 0 but ocunt at 1*/
	return pageIndex+1;
}

Object createTextObject(int objectNumber, char text[]) {
		
	int textLen = strlen(text) + 1;
	char *str = "stream\nBT\n/F1 %d Tf\n%d TL\n30 %d Td\n%s\nET\nendstream";
	int strLength = strlen(str) + 1;
	int streamLength = textLen + strLength + 1 + 2*getDigitStringWidth(FONTSIZE) + getDigitStringWidth(PAGELENGTH);// + 12;
	char stream[streamLength];
	snprintf(stream, streamLength, str, FONTSIZE, FONTSIZE, PAGELENGTH - FONTSIZE - 20, text);
	
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
	strcpy(t[0], "xref\n");
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

	int maxObjects = 100;
	int obsCounter = 0;
	Object obs[maxObjects];

	int pagesRootNumber = ++objectCounter;
	obs[obsCounter] = getPdfRoot(++objectCounter, pagesRootNumber);
	Object *root = &obs[obsCounter];
	obsCounter++;

	/* maximum of 100 pages */
	char **formattedText = malloc(sizeof(char*)*100);
	int numberOfPages = createFormattedText(formattedText, textInput);

	int kidsArray[numberOfPages]; 

	Object *textObject ;
	for (int i = 0; i < numberOfPages; i++) {
		obs[obsCounter]= createTextObject(++objectCounter, formattedText[i]);
		textObject = &obs[obsCounter];
		obsCounter++;

		obs[obsCounter] = createPageObject(++objectCounter, pagesRootNumber, textObject->objectNumber);
		kidsArray[i] = objectCounter;
		obsCounter++;
	}

	obs[obsCounter] = getPagesRoot(1, sizeof(kidsArray)/sizeof(int), kidsArray);
	obsCounter++;

	printf("%s",getStart());

	for (int i = 0; i < obsCounter; i++) {
		printf("%s\n", obs[i].content);
	}

	char **temp = getXref(obs);
	printf("%s", temp[0]);
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
