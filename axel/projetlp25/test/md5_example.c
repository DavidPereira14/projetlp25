#include <stdio.h>
#include <openssl/md5.h>
#include <string.h>
int main() {
    // Le texte à hacher
    const char *text = "Hello, World!";

    // Résultat du hachage MD5
    unsigned char digest[MD5_DIGEST_LENGTH];

    // Calcul du hachage MD5
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, text, strlen(text));
    MD5_Final(digest, &ctx);

    // Afficher le résultat hexadécimal
    printf("MD5 hash of '%s': ", text);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");

    return 0;
}
