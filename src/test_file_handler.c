#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "file_handler.h"
#include "deduplication.h"
#include "backup_manager.h"
#include "network.h"
#include <stdbool.h>
/*
void print_usage(const char *prog_name) {
    printf("Utilisation : %s [OPTIONS]\n", prog_name);
    printf("Options :\n");
    printf("  --backup                : Crée une nouvelle sauvegarde\n");
    printf("  --restore               : Restaure une sauvegarde\n");
    printf("  --list-backups          : Liste les sauvegardes existantes\n");
    printf("  --dry-run               : Effectue une simulation\n");
    printf("  --d-server <IP>         : Adresse IP du serveur de destination\n");
    printf("  --d-port <PORT>         : Port du serveur de destination\n");
    printf("  --s-server <IP>         : Adresse IP du serveur source\n");
    printf("  --s-port <PORT>         : Port du serveur source\n");
    printf("  --dest <CHEMIN>         : Chemin de destination\n");
    printf("  --source <CHEMIN>       : Chemin source\n");
    printf("  -v, --verbose           : Active un affichage détaillé\n");
}

int main(int argc, char *argv[]) {
    bool backup = false, restore = false, list_backups = false, dry_run = false, verbose = false;
    const char *d_server = NULL, *s_server = NULL;
    const char *dest = NULL, *source = NULL;
    int d_port = 0, s_port = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--backup") == 0) {
            if (restore || list_backups) {
                fprintf(stderr, "Erreur : --backup ne peut pas être utilisé avec --restore ou --list-backups\n");
                return EXIT_FAILURE;
            }
            backup = true;
        } else if (strcmp(argv[i], "--restore") == 0) {
            if (backup || list_backups) {
                fprintf(stderr, "Erreur : --restore ne peut pas être utilisé avec --backup ou --list-backups\n");
                return EXIT_FAILURE;
            }
            restore = true;
        } else if (strcmp(argv[i], "--list-backups") == 0) {
            if (backup || restore) {
                fprintf(stderr, "Erreur : --list-backups ne peut pas être utilisé avec --backup ou --restore\n");
                return EXIT_FAILURE;
            }
            list_backups = true;
        } else if (strcmp(argv[i], "--dry-run") == 0) {
            dry_run = true;
        } else if (strcmp(argv[i], "--d-server") == 0) {
            if (++i < argc) {
                d_server = argv[i];
            } else {
                fprintf(stderr, "Erreur : --d-server requiert une adresse IP\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--d-port") == 0) {
            if (++i < argc) {
                d_port = atoi(argv[i]);
            } else {
                fprintf(stderr, "Erreur : --d-port requiert un numéro de port\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--s-server") == 0) {
            if (++i < argc) {
                s_server = argv[i];
            } else {
                fprintf(stderr, "Erreur : --s-server requiert une adresse IP\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--s-port") == 0) {
            if (++i < argc) {
                s_port = atoi(argv[i]);
            } else {
                fprintf(stderr, "Erreur : --s-port requiert un numéro de port\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--dest") == 0) {
            if (++i < argc) {
                dest = argv[i];
            } else {
                fprintf(stderr, "Erreur : --dest requiert un chemin\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--source") == 0) {
            if (++i < argc) {
                source = argv[i];
            } else {
                fprintf(stderr, "Erreur : --source requiert un chemin\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else {
            fprintf(stderr, "Option inconnue : %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (backup) {
        printf("Exécution d'une sauvegarde...\n");
        if (dry_run) printf("Simulation activée.\n");
        if (verbose) printf("Source : %s, Destination : %s\n", source, dest);
        // Appeler votre fonction de sauvegarde ici
    } else if (restore) {
        printf("Restauration en cours...\n");
        if (dry_run) printf("Simulation activée.\n");
        if (verbose) printf("Source : %s, Destination : %s\n", source, dest);
        // Appeler votre fonction de restauration ici
    } else if (list_backups) {
        printf("Liste des sauvegardes...\n");
        // Appeler votre fonction de liste des sauvegardes ici
    } else {
        fprintf(stderr, "Erreur : Aucune action spécifiée.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}




// Fonction pour créer un répertoire si celui-ci n'existe pas
void create_directory_if_needed(const char *dir) {
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        if (mkdir(dir, 0755) != 0) {
            perror("Erreur lors de la création du répertoire");
            exit(EXIT_FAILURE);  // Quitte si une erreur survient
        }
        printf("Répertoire '%s' créé avec succès.\n", dir);
    } else {
        printf("Le répertoire '%s' existe déjà.\n", dir);
    }
}
//Test fonction create_backup
int main(int argc, char *argv[]) {
    // Vérifie si les arguments source et de sauvegarde sont passés
    if (argc > 2) {
        const char *source_dir = argv[1];
        const char *backup_dir = argv[2];

        // Crée les répertoires source et de sauvegarde s'ils n'existent pas
        create_directory_if_needed(source_dir);
        create_directory_if_needed(backup_dir);

        // Appel de la fonction de création de sauvegarde
        create_backup(source_dir, backup_dir);
    } else {
        fprintf(stderr, "Erreur : deux arguments sont nécessaires (source_dir et backup_dir).\n");
        return 1;  // Exit avec une erreur si les arguments sont manquants
    }

    return 0;
}



//Teste fonction write log element
int main() {
    // Créer un élément log fictif pour tester
    log_element elt;

    // Initialisation du chemin, de la date, et du tableau MD5
    elt.path = "/home/user/file1.txt";  // Exemple de chemin de fichier
    elt.date = "2025-01-05 12:30:00";   // Exemple de date

    // Exemple de tableau MD5 (normalement calculé à partir du fichier)
    unsigned char example_md5[MD5_DIGEST_LENGTH] = {
            0xd2, 0xd2, 0xda, 0x67, 0x1a, 0x6f, 0xc2, 0x35,
            0x28, 0x6b, 0xc9, 0x9b, 0x9d, 0xf5, 0x28, 0xc9
    };
    memcpy(elt.md5, example_md5, MD5_DIGEST_LENGTH);  // Copie du tableau MD5 dans la structure

    // Initialisation des pointeurs next et prev (à NULL pour l'instant)
    elt.next = NULL;
    elt.prev = NULL;

    // Ouvrir un fichier de log pour écrire
    FILE *logfile = fopen(".backup_log", "w");
    if (logfile == NULL) {
        perror("Erreur lors de l'ouverture du fichier de log");
        return EXIT_FAILURE;
    }

    // Appeler la fonction write_log_element pour écrire l'élément
    write_log_element(&elt, logfile);

    // Fermer le fichier de log
    fclose(logfile);

    return 0;
}


//Test write_log_element et  updtade_backup_log
int main() {
    const char *logfile = ".backup_log";

    // Créer les fichiers file1.txt et file2.txt pour tester
    FILE *file1 = fopen("file1.txt", "w");
    if (file1 == NULL) {
        perror("Erreur lors de la création de file1.txt");
        return EXIT_FAILURE;
    }
    fprintf(file1, "Contenu de file1.txt\n");
    fclose(file1);

    FILE *file2 = fopen("file2.txt", "w");
    if (file2 == NULL) {
        perror("Erreur lors de la création de file2.txt");
        return EXIT_FAILURE;
    }
    fprintf(file2, "Contenu de file2.txt\n");
    fclose(file2);

    // Créer un fichier de log simulé
    FILE *file = fopen(logfile, "w");
    if (file == NULL) {
        perror("Erreur lors de la création du fichier de log");
        return EXIT_FAILURE;
    }

    // Écrire quelques lignes dans le fichier .backup_log
    fprintf(file, "/mnt/c/Users/david/OneDrive/Documents/GitHub/projetlp25/file1.txt;2025-01-05 12:30:00;d2d2da671a6fc235286bc99b9df528c9\n");
    fprintf(file, "/mnt/c/Users/david/OneDrive/Documents/GitHub/projetlp25/file2.txt;2025-01-05 12:31:00;5f4dcc3b5aa765d61d8327deb882cf99\n");
    fprintf(file, "/mnt/c/Users/david/OneDrive/Documents/GitHub/projetlp25/file_non_existant.txt;2025-01-05 12:32:00;1234567890abcdef1234567890abcdef\n");

    fclose(file);

    // Initialiser la structure de logs (log_t) pour simuler l'état avant modification
    log_t logs = { NULL, NULL };

    // Appeler la fonction pour mettre à jour le fichier de log
    update_backup_log(logfile, &logs);

    // Lire et afficher le contenu du fichier après mise à jour
    file = fopen(logfile, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier pour lecture");
        return EXIT_FAILURE;
    }

    printf("Contenu mis à jour du fichier .backup_log :\n");
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);  // Fermer le fichier après lecture

    return 0;
}
 */