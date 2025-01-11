#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include "file_handler.h"
#include "deduplication.h"

extern bool verbose;

// Fonction pour lire le fichier de log de sauvegarde
log_t read_backup_log(const char *logfile) {
    printf("Read_backup_log\n");
    log_t logs = {NULL, NULL};  // Initialiser la liste vide
    FILE *file = fopen(logfile, "r");  // Ouvrir le fichier .backup_log en mode lecture

    if (!file) {
        perror("Erreur lors de l'ouverture du fichier de sauvegarde");
        return logs;  // Retourne une liste vide en cas d'erreur
    }
    if (verbose){
        printf("Lecture du fichier: %s\n", logfile);
    }

    char line[1024];  // Buffer pour lire chaque ligne
    while (fgets(line, sizeof(line), file)) {
        printf("%s\n", line);

        // Analyser la première partie : le chemin complet (YYYY-MM-DD-hh:mm:ss.sss/folder1/file1)
        char *path = strtok(line, ";");  // Le chemin est avant le premier point-virgule
        if (!path) {
            printf("Impossible de recuperer path.\n");
            continue;  // Continuer à la ligne suivante si on ne peut pas récupérer le path
        }

        // Analyser la deuxième partie : la date de dernière modification (mtime)
        char *mtime_str = strtok(NULL, ";");
        if (!mtime_str) {
            printf("Impossible de recuperer mtime.\n");
            continue;  // Continuer à la ligne suivante si mtime est manquant
        }

        // Analyser la troisième partie : le MD5
        char *md5_str = strtok(NULL, ";");
        if (!md5_str) {
            printf("Impossible de recuperer md5.\n");
            continue;  // Continuer à la ligne suivante si md5 est manquant
        }

        // Convertir le MD5 de la chaîne hexadécimale en binaire
        unsigned char md5[MD5_DIGEST_LENGTH];
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
            sscanf(md5_str + 2 * i, "%2hhx", &md5[i]);  // Convertir deux caractères hexadécimaux en un octet
        }

        // Créer un nouvel élément de log
        log_element *new_log = (log_element *)malloc(sizeof(log_element));
        if (!new_log) {
            perror("Erreur d'allocation mémoire pour un nouvel élément de log");
            fclose(file);
            return logs;  // Retourne la liste actuelle, vide en cas d'erreur
        }

        // Copier les données dans le nouvel élément
        new_log->path = strdup(path);
        new_log->date = strdup(mtime_str);
        memcpy(new_log->md5, md5, MD5_DIGEST_LENGTH);

        // Initialiser le next et prev du nouvel élément
        new_log->next = NULL;
        new_log->prev = logs.tail;

        // Ajouter l'élément à la liste doublement chaînée
        if (logs.tail) {
            logs.tail->next = new_log;
        } else {
            logs.head = new_log;  // Si la liste est vide, cet élément devient la tête
        }
        logs.tail = new_log;  // Ce nouvel élément devient la queue de la liste
    }

    if (verbose){
        printf("Fichier %s lu avec succes.\n", logfile);
    }

    fclose(file);  // Fermer le fichier après lecture
    return logs;  // Retourner la liste de logs
}


// Fonction pour mettre à jour une ligne du fichier .backup_log
void update_backup_log(const char *logfile, log_t *logs) {
    /*
     * @param: logfile - le chemin vers le fichier .backup_log
     *         logs - la liste de toutes les lignes du fichier .backup_log sauvegardée dans une structure log_t
     */
    printf("Update_backup_log\n");
    // Ouvre le fichier .backup_log en mode lecture
    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    // Stocke toutes les lignes du fichier
    char *lines[1024];  // Tableau pour stocker les lignes du fichier
    int line_count = 0;
    char line[1024];     // Buffer pour lire chaque ligne

    // Parcours du fichier et stockage des lignes
    while (fgets(line, sizeof(line), file)) {
        // Allocation de mémoire pour chaque ligne
        lines[line_count] = strdup(line);
        if (!lines[line_count]) {
            perror("Erreur d'allocation mémoire");
            fclose(file);
            return;
        }
        line_count++;
    }

    fclose(file);  // Ferme le fichier après la lecture

    // Ouvre à nouveau le fichier en mode écriture pour le réécrire sans les anciennes lignes
    file = fopen(logfile, "w");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier en écriture");
        return;
    }

    log_element *current = logs->head;  // Pointeur vers le premier élément de la liste de logs
    int found = 0;  // Variable pour indiquer si la ligne a été trouvée et modifiée

    // Parcourir les lignes existantes et comparer avec les logs à mettre à jour
    for (int i = 0; i < line_count; i++) {
        char *log_path = strtok(lines[i], ";");  // Extraire le chemin du fichier de chaque ligne
        if (log_path) {
            log_element *temp = current;
            // Comparer le chemin du fichier avec le log actuel
            while (temp != NULL) {
                if (strcmp(temp->path, log_path) == 0) {
                    // Si le fichier est trouvé, mettre à jour les informations (par exemple, la date et le MD5)
                    fprintf(file, "%s;%s;%s\n", temp->path, temp->date, temp->md5);
                    found = 1;  // Indiquer que le fichier a été trouvé et modifié
                    break;
                }
                temp = temp->next;
            }

            // Si le fichier n'a pas été trouvé dans la liste de logs, conserver l'ancienne ligne
            if (!found) {
                fprintf(file, "%s", lines[i]);
            }

            // Réinitialiser la variable found pour le prochain fichier
            found = 0;
        }
    }

    // Écrire les nouvelles lignes qui n'existent pas déjà dans le fichier
    while (current != NULL) {
        fprintf(file, "%s;%s;%s\n", current->path, current->date, current->md5);
        current = current->next;
    }

    // Ferme le fichier après la réécriture
    fclose(file);

    // Libérer la mémoire allouée pour les lignes
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
}

void write_log_element(log_element *elt, FILE *logfile) {
    if (!logfile || !elt) {
        fprintf(stderr, "Fichier ou log invalide.\n");
        return;
    }

    // Écrire le chemin du fichier (YYYY-MM-DD-hh:mm:ss.sss/folder1/file1)
    fprintf(logfile, "%s;", elt->path);

    // Écrire la date de dernière modification (mtime)
    fprintf(logfile, "%s;", elt->date);

    // Écrire le MD5 en format hexadécimal
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        fprintf(logfile, "%02x", elt->md5[i]);
    }

    // Ajouter un saut de ligne après chaque log
    fprintf(logfile, "\n");
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

    //printf("Le fichier '%s' a été copié vers '%s'.\n", src, dest);

    fclose(src_f);  // Ferme le fichier source
    fclose(dest_f);  // Ferme le fichier destination
}
