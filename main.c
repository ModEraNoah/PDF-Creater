#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static uint objectCounter = 0;

typedef struct {
	uint objectNumber;
	char *content;
	uint length;
} Object;

char* getStart() {
	return "%PDF-1.7\n";
}

Object getPdfRoot() {
	Object temp = {++objectCounter,"<</Type /Catalog /Pages 2 0 R>>"};
	return temp;
}

Object getPagesRoot() {
	Object temp = {++objectCounter,"<</Type /Pages /Kids [3 0 R] /Count 1>>"};
	return temp;
}

char** getXref(Object obs[]) {
	char ** t = malloc (10 * sizeof(char*));

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
		printf("\toffset: %d\n", offset);
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
	obs[0] = getPdfRoot();
	obs[1] = getPagesRoot();
	printf("%s",getStart());

	printf("%i 0 obj\n", obs[0].objectNumber);
	printf("%s\n", obs[0].content);
	printf("endobj\n");

	printf("%i 0 obj\n", obs[1].objectNumber);
	printf("%s\n", obs[1].content);
	printf("endobj\n");

	// printf("before calling getXref");
	char **temp = getXref(obs);
	printf("xref\n");
	printf("%s", temp[0]);
	printf("trailer\n");

	free(temp);

	return 0;
}
