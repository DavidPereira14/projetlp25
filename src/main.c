#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "file_handler.h"
#include "deduplication.h"
#include "backup_manager.h"
#include "network.h"
#include <stdbool.h>

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
        create_backup(source,dest);
    } else if (restore) {
        printf("Restauration en cours...\n");
        if (dry_run) printf("Simulation activée.\n");
        if (verbose) printf("Source : %s, Destination : %s\n", source, dest);
        restore_backup(source,dest);
    } else if (list_backups) {
        printf("Liste des sauvegardes...\n");
        list_backups(source);
    } else {
        fprintf(stderr, "Erreur : Aucune action spécifiée.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


