#include <unistd.h>

int main() {
    if (setuid(31337) == -1) {
        perror("setuid");
        return 1;
    }
    execlp("bash", "bash", NULL);
}
