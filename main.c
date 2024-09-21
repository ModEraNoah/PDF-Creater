#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define FONTSIZE 14

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
	while ((digit/(pow(10, charCounter))) >= 0.1) {
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
	char *content= (char*) malloc(length  * sizeof(char));
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
	char **content = craftObjectContent(objectNumber,"<</Type /Pages /Kids [3 0 R] /Count 1>>");

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

Object createTextObject(int objectNumber, char text[]) {
		
	int textLen = strlen(text) + 1;
	char *str = "stream\nBT\n/F1 %d Tf\n%d TL\n70 300 TD\n(%s) Tj\nET\nendstream";
	int strLength = strlen(str) + 1;
	// int streamLength = 54+textLen + 2 + 12;
	int streamLength = textLen + strLength + 1 + 2*getDigitStringWidth(FONTSIZE);// + 12;
	char stream[streamLength];
	snprintf(stream, streamLength, str, FONTSIZE, FONTSIZE, text);

	// TODO: ERROR at dict calculation
	int charCounter = getDigitStringWidth(streamLength);
	char dict[13+charCounter + 1] ;
	snprintf(dict, 13+charCounter+1, "<</Length %d>>\n", streamLength);

	char contentString[streamLength + 13 + charCounter + 1];
	strcat(contentString,dict);
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
	// free(obs[0].content);

	// printf("%i 0 obj\n", obs[1].objectNumber);
	// printf("%s\n", obs[1].content);
	// printf("endobj\n");
	printf("%s\n", obs[1].content);
	// free(obs[1].content);

	int parent = objectCounter;
	obs[2] = createPageObject(++objectCounter, parent, parent+2);
	printf("%s\n", obs[2].content);

	obs[3]= createTextObject(++objectCounter, "Hallo Welt!");
	printf("%s\n", obs[3].content);
	// free(obs[2].content);

	// // printf("before calling getXref");
	char **temp = getXref(obs);
	printf("%s", temp[0]);
	printf("<</Size 5 /Root 1 0 R>>\n");
	printf("startxref\n");
	printf("%d\n", currentLength);
	printf("%%%%EOF\n");

	free(temp);

	return 0;
}
