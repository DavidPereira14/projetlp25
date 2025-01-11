#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include "file_handler.h"
#include "deduplication.h"

extern bool verbose;

// Fonction permettant de lire un élément du fichier .backup_log
log_t read_backup_log(const char *logfile){
    /* Implémenter la logique pour la lecture d'une ligne du fichier ".backup_log"
    * @param: logfile - le chemin vers le fichier .backup_log
    * @return: une structure log_t
    */
    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier .backup_log");
        exit(EXIT_FAILURE); // Quitte en cas d'erreur d'ouverture
    }

    // Initialise une liste vide pour stocker les logs
    log_t log_list = { NULL, NULL };

    char line[1024];  // Tableau pour stocker chaque ligne du fichier

    while (fgets(line, sizeof(line), file)) {
        // Crée un nouvel élément pour la liste chaînée
        log_element *new_element = malloc(sizeof(log_element));
        if (!new_element) {
            perror("Erreur d'allocation mémoire");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Parse la ligne en utilisant strtok pour extraire les données
        char *path = strtok(line, ";");
        char *time = strtok(NULL, ";");
        char *md5_str = strtok(NULL, "\n");  // "\n" pour ignorer la fin de ligne

        if (!path || !time || !md5_str) {
            perror("Erreur lors de la récupération des données");
            free(new_element); // Libère l'élément en cas d'erreur
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Remplir l'élément avec les valeurs extraites
        new_element->path = strdup(path);  // Duplication pour éviter les problèmes de gestion de mémoire
        new_element->date = strdup(time);

        // Convertir la chaîne MD5 en tableau d'octets
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            sscanf(md5_str + (i * 2), "%2hhx", &new_element->md5[i]);
        }

        // Ajout de l'élément à la liste chaînée
        new_element->next = NULL;
        new_element->prev = log_list.tail;

        if (log_list.tail) {
            log_list.tail->next = new_element;  // Relie le précédent élément
        }
        log_list.tail = new_element;  // Le nouvel élément devient le dernier

        if (log_list.head == NULL) {
            log_list.head = new_element;  // Si la liste était vide, le nouvel élément est aussi le premier
        }

        // Si verbose est activé, afficher le log ajouté
        if (verbose) {
            printf("Ajout du log: %s; %s; %s;\n", new_element->path, new_element->date, md5_str);
        }
    }

    fclose(file);
    return log_list;  // Retourne la liste chaînée remplie
}

// Fonction permettant de mettre à jour une ligne du fichier .backup_log
void update_backup_log(const char *logfile, log_t *logs) {
    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    char **lines = NULL; // Tableau dynamique pour stocker les lignes valides
    int line_count = 0;
    char line[1024];

    while (fgets(line, sizeof(line), file)) {
        char *line_copy = strdup(line); // Copie de la ligne pour stockage
        if (!line_copy) {
            perror("Erreur d'allocation mémoire");
            break;
        }

        // Vérifie si le fichier référencé existe
        char *path_in_file = strtok(line, ";");
        if (path_in_file && access(path_in_file, F_OK) == 0) {
            // Ajoute la ligne valide au tableau
            char **temp = realloc(lines, (line_count + 1) * sizeof(char *));
            if (!temp) {
                perror("Erreur d'allocation mémoire");
                free(line_copy);
                break;
            }
            lines = temp;
            lines[line_count++] = line_copy;

            if (verbose) {
                printf("Ligne valide: %s\n", line_copy);
            }

        } else {
            free(line_copy); // Libère la ligne si invalide
        }
    }

    fclose(file);

    // Ouvre le fichier en mode écriture pour réécrire les lignes valides
    file = fopen(logfile, "w");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier en écriture");
        // Libère les lignes collectées avant de quitter
        for (int i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        free(lines);
        return;
    }

    // Écrit les lignes valides dans le fichier
    for (int i = 0; i < line_count; i++) {
        fprintf(file, "%s", lines[i]); // Pas besoin de \n, déjà inclus par fgets
        free(lines[i]);
    }

    free(lines);
    fclose(file);

    if (verbose) {
        printf("Mise à jour du fichier .backup_log avec %d lignes valides.\n", line_count);
    }
}

void write_log_element(log_element *elt, FILE *logfile) {
    if (!logfile || !elt || !elt->path || !elt->date || !elt->md5) {
        fprintf(stderr, "Fichier ou log invalide.\n");
        return;
    }

    // Calculer la somme des octets du MD5
    unsigned int md5_sum = 0;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        md5_sum += elt->md5[i];
    }

    // Écrire les informations dans le fichier de log
    char md5_hex[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        sprintf(md5_hex + i * 2, "%02x", elt->md5[i]);
    }

    // Écriture dans le fichier
    if (fprintf(logfile, "%s;%s;%u\n", elt->path, elt->date,md5_sum) < 0) {
        perror("Erreur d'écriture dans le fichier");
        return;
    }

    if (verbose) {
        printf("Ecriture du log: %s;%s;%u\n", elt->path, elt->date, md5_sum);
    }

    return;

}

void list_files(const char *path){
    /* Implémenter la logique pour lister les fichiers présents dans un répertoire
    */
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Erreur lors de l'ouverture du répertoire");
        return;
    }

    struct dirent *entry;

    // Parcourt les entrées du répertoire
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        printf("%s\n", entry->d_name);

        if (verbose) {
            printf("Fichier trouvé dans le répertoire: %s\n", entry->d_name);
        }
    }

    // Ferme le répertoire
    if (closedir(dir) != 0) {
        perror("Erreur lors de la fermeture du répertoire");
    }
}

void copy_file(const char *src, const char *dest) {
    FILE *src_f = fopen(src, "rb");  // Ouvre le fichier source en mode binaire
    if (src_f == NULL) {
        perror("Erreur d'ouverture du fichier source");
        return;
    }

    FILE *dest_f = fopen(dest, "wb");  // Ouvre le fichier destination en mode binaire
    if (dest_f == NULL) {
        perror("Erreur d'ouverture du fichier destination");
        fclose(src_f);
        return;
    }

    char buffer[4096];  // Buffer pour lire et écrire des données
    size_t bytes_read;

    // Lire et copier le contenu du fichier source dans le fichier destination
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_f)) > 0) {
        fwrite(buffer, 1, bytes_read, dest_f);
    }

    printf("Le fichier '%s' a été copié vers '%s'.\n", src, dest);

    fclose(src_f);  // Ferme le fichier source
    fclose(dest_f);  // Ferme le fichier destination
}
