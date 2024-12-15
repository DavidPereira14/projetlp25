#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include "file_handler.h"
#include "deduplication.h"
#include <unistd.h>

// Fonction permettant de lire un élément du fichier .backup_log
log_t read_backup_log(const char *logfile) {
    /* Implémenter la logique pour la lecture d'une ligne du fichier ".backup_log"
    * @param: logfile - le chemin vers le fichier .backup_log
    * @return: une structure log_t
    */
    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier .backup_log");
        exit(EXIT_FAILURE); // Quitte en cas d'erreur d'ouverture
    }

    log_t log_list = { NULL, NULL }; // Initialise une liste vide

    char line[1024]; // Tableau pour stocker chaque ligne du fichier
    while (fgets(line, sizeof(line), file)) {
        log_element *new_element = malloc(sizeof(log_element));
        if (!new_element) {
            perror("Erreur d'allocation mémoire");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Parse la ligne
        char *path = strtok(line, ";");
        char *time = strtok(NULL, ";");
        char *md5_str = strtok(NULL, "\n");

        if (!path || !time || !md5_str) {
            fprintf(stderr, "Erreur lors de l'analyse d'une ligne de log\n");
            free(new_element);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Allocation sécurisée pour les données
        new_element->path = strdup(path);
        if (!new_element->path) {
            perror("Erreur d'allocation mémoire pour path");
            free(new_element);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        new_element->date = strdup(time);
        if (!new_element->date) {
            perror("Erreur d'allocation mémoire pour date");
            free((void *)new_element->path);
            free(new_element);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Convertir la chaîne MD5 en tableau d'octets
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            sscanf(md5_str + (i * 2), "%2hhx", &new_element->md5[i]);
        }

        // Initialisation des pointeurs de l'élément
        new_element->next = NULL;        // Le nouvel élément n'a pas encore de suivant
        new_element->prev = log_list.tail; // Le précédent élément est le dernier de la liste actuelle

        // Mise à jour de la liste chaînée
        if (log_list.tail) {
            log_list.tail->next = new_element; // Relier l'ancien dernier élément au nouvel élément
        }
        log_list.tail = new_element; // Le nouvel élément devient le dernier de la liste

        // Si la liste était vide, le nouvel élément devient aussi le premier
        if (log_list.head == NULL) {
            log_list.head = new_element;
        }
    }

    fclose(file);
    return log_list;
}

// Fonction permettant de mettre à jour une ligne du fichier .backup_log
void update_backup_log(const char *logfile, log_t *logs) {
    /* Implémenter la logique de modification d'une ligne du fichier ".backup_log"
     * @param: logfile - le chemin vers le fichier .backup_log
     *         logs - la liste de toutes les lignes du fichier .backup_log sauvegardée dans une structure log_t
     */

    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    char *lines[1024];
    int line_count = 0;
    char line[1024];

    // Parcours des lignes pour vérifier leur validité
    while (fgets(line, sizeof(line), file)) {
        char *path_in_file = strtok(line, ";");

        if (path_in_file && access(path_in_file, F_OK) == -1) {
            continue; // Ignorer si le fichier n'existe pas
        }

        lines[line_count] = strdup(line);
        if (!lines[line_count]) {
            perror("Erreur d'allocation mémoire");
            fclose(file);
            return;
        }
        line_count++;
    }
    fclose(file);

    // Réécriture du fichier
    file = fopen(logfile, "w");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier en écriture");
        for (int i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        return;
    }

    for (int i = 0; i < line_count; i++) {
        fprintf(file, "%s", lines[i]);
        free(lines[i]);
    }

    fclose(file);
}

void write_log_element(log_element *elt, FILE *logfile) {
    /* Implémenter la logique pour écrire un élément log de la liste chaînée log_element dans le fichier .backup_log
     * @param: elt - un élément log à écrire sur une ligne
     *         logfile - un pointeur vers le fichier ouvert
     */

    if (!logfile) {
        perror("Le fichier est NULL");
        return;
    }

    char md5_str[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_str[i * 2], "%02x", elt->md5[i]);
    }

    fprintf(logfile, "%s;%s;%s\n", elt->path, elt->date, md5_str);
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