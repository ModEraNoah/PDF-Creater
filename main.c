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
	/*
	int charCounter = 1;
	while (objectNumber/(charCounter * 10)) {
		charCounter++;
	}

	char objectBeginning[charCounter + 8];
	snprintf(objectBeginning, sizeof(objectBeginning), "%i 0 obj\n", objectNumber);

	char *objectEnding = "\nendobj";
	char *rootCatalog = "<</Type /Catalog /Pages 2 0 R>>";

	int length = strlen(objectBeginning) + strlen(rootCatalog) + strlen(objectEnding);
	char *content= (char*) malloc((length + 8) * sizeof(char));

	strcat(content, objectBeginning);
	strcat(content, rootCatalog);
	strcat(content, objectEnding);
	*/
	char **content = craftObjectContent(objectNumber, "<</Type /Catalog /Pages 2 0 R>>");

	Object temp = {objectNumber, *content};
	return temp;
}

Object getPagesRoot(int objectNumber) {
	Object temp = {objectNumber,"<</Type /Pages /Kids [3 0 R] /Count 1>>"};
	return temp;
}

char** getXref(Object obs[]) {
	char ** t = malloc (1 * sizeof(char*));

	char num[4];
	snprintf(num, sizeof(num), "%d\n", objectCounter+1);
	char temp[6] = "0 ";
	strcat(temp, num);
	t[0] = malloc(1000 * sizeof(char));
	strcat(t[0], temp);
	strcat(t[0],"0000000000 65535 f\n");
	int currentLength = 10;
	for (int i = 0; i < objectCounter; i++) {
		int offset;
		if (i == 0)
			offset = currentLength;
		else 
			offset = 16 + currentLength;
		char numb[11];
		snprintf(numb, sizeof(numb), "%010d", offset);
		currentLength += offset;
		currentLength += strlen(obs[i].content);
		strcat(t[0], numb);
		strcat(t[0], " 00000 n\n");
		
		// currentLength = strlen(t[0]);
	}
	return t;
}

int main(void) {
	Object obs[2];
	obs[0] = getPdfRoot(++objectCounter);
	obs[1] = getPagesRoot(++objectCounter);
	printf("%s",getStart());

	printf("%s\n", obs[0].content);
	free(obs[0].content);

	// printf("%i 0 obj\n", obs[1].objectNumber);
	// printf("%s\n", obs[1].content);
	// printf("endobj\n");
	// printf("%s\n", obs[1].content);
	// free(obs[1].content);
	//
	// // printf("before calling getXref");
	// char **temp = getXref(obs);
	// printf("xref\n");
	// printf("%s", temp[0]);
	// printf("trailer\n");
	//
	// free(temp);

	return 0;
}
