#include <syslog.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    syslog(LOG_INFO, "use syslog print LOG_INFO level msg.\n");
    
    openlog("test-syslog", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "print LOG_INFO level msg after openlog function.\n");

    setlogmask(LOG_UPTO(LOG_WARNING));
    syslog(LOG_INFO, "use syslog print LOG_INFO level msg.\n");
    syslog(LOG_WARNING, "use syslog print LOG_WARNING level msg.\n");
    syslog(LOG_ERR, "use syslog print LOG_ERR level msg.\n");

    closelog();
    syslog(LOG_ERR, "print log after closelog.\n");


    // Output:
    // Jan 18 15:11:03 ubuntu16 test_syslog: use syslog print LOG_INFO level msg.
    // Jan 18 15:11:03 ubuntu16 test-syslog[8285]: print LOG_INFO level msg after openlog function.
    // Jan 18 15:11:03 ubuntu16 test-syslog[8285]: use syslog print LOG_WARNING level msg.
    // Jan 18 15:11:03 ubuntu16 test-syslog[8285]: use syslog print LOG_ERR level msg.
    return 0;
}