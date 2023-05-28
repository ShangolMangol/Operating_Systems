#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <signal.h>

using namespace std;

int main() {
    int x = get_weight();
    cout << "weight: " << x << endl;
    assert(x == 0);
    x = set_weight(5);
    cout << "set_weight returns: " << x << endl;
    assert(x == 0);
    x = get_weight();
    cout << "new weight: " << x << endl;
    assert(x == 5);
    x = get_ancestor_sum();
    cout << "ancestor sum: " << x << endl;
    assert(x == 5);
    int child_pid = fork();
    if(child_pid == 0)
    {
        // child
        int child_weight = get_weight();
        cout << "child weight: " << child_weight << endl;
        assert(child_weight == 5);
        int child_ancestor_sum = get_ancestor_sum();
        cout << "child ancestor sum: " << child_ancestor_sum << endl;
        assert(child_ancestor_sum == 10);
        int child_heaviest_descendant = get_heaviest_descendant();
        cout << "child heaviest descendant: " << child_heaviest_descendant << endl;
        assert(child_heaviest_descendant == -ECHILD);
        child_weight = set_weight(10);
        cout << "child set_weight returns: " << child_weight << endl;
        assert(child_weight == 0);
        child_weight = get_weight();
        cout << "child new weight: " << child_weight << endl;
        assert(child_weight == 10);
        child_ancestor_sum = get_ancestor_sum();
        cout << "child ancestor sum: " << child_ancestor_sum << endl;
        assert(child_ancestor_sum == 15);
        while (1);
    }
    sleep(2);

    pid_t y = get_heaviest_descendant();
    cout << "child pid: " << child_pid << endl;
    cout << "heaviest descendant: " << y << endl;
    assert(y == child_pid);
    kill(child_pid, SIGKILL);
    cout << "===== SUCCESS =====" << endl;
    return 0;
}

