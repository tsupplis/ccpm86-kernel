/* Local Concurrent launch API, derived from util.lib (MIT).
 * Non-reentrant: only the native execution adapter calls this interface.
 */
#ifndef FM_SPAWN_H
#define FM_SPAWN_H
int p_spawn(char *command);
unsigned spawn_data(void);
int p_cli(char *command);
int ms_p_cli(char *opened_fcb, char *command);
int pcorder(char *order);
int pcsysdisk(void);
int pcclean(void);
int getpriority(void);
int getppriority(void);
int p_priority(int priority);
int conattach(void);
#endif
