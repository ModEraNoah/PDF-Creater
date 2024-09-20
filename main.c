#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static uint objectCounter = 0;

typedef struct {
	uint objectNumber;
	char *content;
} Object;

char* getStart() {
	return "%PDF-1.7\n";
}


char** craftObjectContent(int objectNumber, char contentString[]) {
	char **res = malloc(sizeof(char*));

	int charCounter = 1;
	while (objectNumber/(charCounter * 10)) {
		charCounter++;
	}

	char objectBeginning[charCounter + 8];
	snprintf(objectBeginning, sizeof(objectBeginning), "%i 0 obj\n", objectNumber);

	char *objectEnding = "\nendobj";

	int length = strlen(objectBeginning) + strlen(contentString) + strlen(objectEnding);
	char *content= (char*) malloc((length + 8) * sizeof(char));
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
	int charCounter = 1;
	while (parent/(charCounter * 10)) {
		charCounter++;
	}
	additionalLen = charCounter;
	charCounter = 1;
	while (contents/(charCounter * 10)) {
		charCounter++;
	}
	additionalLen += charCounter;
	char contentString[44 + additionalLen];
	snprintf(contentString, 44 + additionalLen, "<</Type /Page /Parent %d 0 R /Contents %d 0 R>>", parent, contents);

	char **content = craftObjectContent(objectNumber, contentString);

	Object temp = {objectNumber,*content};
	return temp;
}

Object createTextObject(int objectNumber, char text[]) {
		
	int textLen = strlen(text);
	int streamLength = 48+textLen+1;
	char stream[streamLength];
	snprintf(stream, streamLength, "stream\nBT\n/F1 14 Tf\n70 300 TD\n(%s) Tj\nET\nendstream", text);

	int charCounter = 1;
	while (streamLength/(charCounter * 10)) {
		charCounter++;
	}
	char dict[13+charCounter] ;
	snprintf(dict, 13+charCounter, "<</Length %d>>\n", streamLength);

	char contentString[streamLength + 13 + charCounter];
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
	printf("<</Size 4 /Root 1 0 R>>\n");
	printf("startxref\n");
	printf("%d\n", currentLength);
	printf("%%%%EOF\n");

	free(temp);

	return 0;
}
