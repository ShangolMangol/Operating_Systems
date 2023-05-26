#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <sys/wait.h>

using namespace std;

int main() {
	
	cout << "===== BEGIN =====" << endl;

	// SEGEL Tests
    int x = get_weight();
	assert(x == 0);
    x = set_weight(5);
	assert(x == 0);
    x = get_weight();
	assert(x == 5);
	
	cout << "===== PASSED SEGEL TESTS =====" << endl;

	// Test negative weight
	x = set_weight(-10);
	assert(x == -1);
	x = get_weight();
	assert(x == 5);

	cout << "===== PASSED NEGATIVE WEIGHT =====" << endl;

	// Test fork copy works
	pid_t pid = fork();
	if (pid == 0) {
		// Child
		x = get_weight();
		assert(x == 5);
		x = set_weight(-10);
		assert(x == -1);
		exit(0);
	}
	else {
		// Parent
		wait(NULL);
	}
	x = get_weight();
	assert(x == 5);

	cout << "===== PASSED FORK TEST =====" << endl;

	// Test get_ancestor_sum

	// Test all ancestors have weight 0
	x = get_ancestor_sum();
	assert(x == 5);

	// Test with one child

	pid = fork();
	if (pid == 0) {
		// Child
		x = get_ancestor_sum();
		assert(x == 10);
		exit(0);
	} else {
		// Parent
		wait(NULL);
	}

	// Test with more descendants
	x = get_ancestor_sum();
	assert(x == 5);

	pid = fork();
	if (pid == 0) {
		// Child
		x = get_ancestor_sum();
		assert(x == 10);
		x = set_weight(10);
		assert(x == 0);
		
		pid_t pid2 = fork();
		if (pid2 == 0) {
			// Grandchild
			x = get_ancestor_sum();
			assert(x == 25);
			x = set_weight(15);
			assert(x == 0);
			x = get_ancestor_sum();
			assert(x == 30);

			pid_t pid3 = fork();
			if (pid3 == 0) {
				// Great Grandchild
				x = get_ancestor_sum();
				assert(x == 45);
				exit(0);
			} else {
				// Grandchild
				waitpid(pid3, NULL, 0);
				exit(0);
			}
		} else {
			// Child
			waitpid(pid2, NULL, 0);
			exit(0);
		} 
	} else {
		// Parent
		waitpid(pid, NULL, 0);
	}
	x = get_ancestor_sum();
	assert(x == 5);

	cout << "===== PASSED ANCESTOR SUM TEST =====" << endl;


	// Test get_heaviest_descendant

	// Test no descendants
	x = get_heaviest_descendant();
	assert(x == -1);

	// Test one layer of descendants

	pid = fork();
	if (pid == 0) {
		// Child1
		x = set_weight(10);
		assert(x == 0);
		exit(0);
	}

	pid_t pid2 = fork();
	if (pid2 == 0) {
		// Child2
		x = set_weight(20);
		assert(x == 0);
		exit(0);
	}

	pid_t pid3 = fork();
	if (pid3 == 0) {
		// Child3
		x = set_weight(30);
		assert(x == 0);
		exit(0);
	}

	// wait for children to set weight
	sleep(1);
	pid_t heaviest = get_heaviest_descendant();
	assert(heaviest == pid3);

	waitpid(pid, NULL, 0);
	waitpid(pid2, NULL, 0);
	waitpid(pid3, NULL, 0);


	// Test multiple layers of descendants

	int fd[2];
	if(pipe(fd) == -1) {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	if (pid == 0) {
		// Child1
		x = set_weight(10);
		assert(x == 0);
		pid2 = fork();
		if (pid2 == 0) {
			// Grandchild1
			x = set_weight(30);
			assert(x == 0);
			pid_t mypid = getpid();
			close(fd[0]);
			if(write(fd[1], &mypid, sizeof(mypid)) == -1) {
				perror("write");
				exit(1);
			}
			close(fd[1]);	
			pid3 = fork();
			if (pid3 == 0) {
				// Great Grandchild1
				x = set_weight(20);
				assert(x == 0);
				exit(0);
			} else {
				// Grandchild1
				sleep(2);
				waitpid(pid3, NULL, 0);
				exit(0);
			}
			exit(0);
		} else {
			// Child1
			close(fd[0]);
			close(fd[1]);
			sleep(2);
			waitpid(pid2, NULL, 0);
			exit(0);
		}
	} 

	// also check that father is not included in heaviest descendant
	x = set_weight(40);
	assert(x == 0);

	// wait for children to set weight
	sleep(1);
	pid_t heaviest_pid;
	close(fd[1]);
	if (read(fd[0], &heaviest_pid, sizeof(heaviest_pid)) == -1) {
		perror("read");
		exit(1);
	}
	close(fd[0]);
	heaviest = get_heaviest_descendant();
	assert(heaviest == heaviest_pid);

	// wait for children to finish
	waitpid(pid, NULL, 0);


	// Test multiple layers of descendants with multiple children

	if(pipe(fd) == -1) {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	if (pid == 0) {
		// Child1
		close(fd[0]);
		close(fd[1]);
		x = set_weight(10);
		assert(x == 0);
		pid2 = fork();
		if (pid2 == 0) {
			// Grandchild1
			x = set_weight(30);
			assert(x == 0);	
			pid3 = fork();
			if (pid3 == 0) {
				// Great Grandchild1
				x = set_weight(20);
				assert(get_weight() == 20);
				exit(0);
			} else {
				// Grandchild1
				sleep(2);
				waitpid(pid3, NULL, 0);
				exit(0);
			}
			exit(0);
		} else {
			// Child1
			sleep(2);
			waitpid(pid2, NULL, 0);
			exit(0);
		}
	}

	pid_t pid4 = fork();
	if (pid4 == 0) {
		// Child2
		close(fd[0]);
		close(fd[1]);
		x = set_weight(50);
		assert(x == 0);
		pid_t pid5 = fork();
		if (pid5 == 0) {
			// Grandchild2
			x = set_weight(60);
			assert(get_weight() == 60);
			exit(0);
		} else {
			// Child2
			sleep(2);
			waitpid(pid5, NULL, 0);
			exit(0);
		}
	}

	pid_t pid6 = fork();
	if (pid6 == 0) {
		// Child3
		close(fd[0]);
		x = set_weight(70);
		assert(x == 0);
		pid_t pid7 = fork();
		if (pid7 == 0) {
			// Grandchild3
			x = set_weight(80);
			assert(x == 0);
			pid_t pid8 = fork();
			if (pid8 == 0) {
				// Great Grandchild3
				x = set_weight(90);
				assert(get_weight() == 90);

				pid_t pid9 = fork();
				if (pid9 == 0) {
					// Great Great Grandchild3
					x = set_weight(100);
					assert(get_weight() == 100);
					exit(0);
				} else {
					// Great Grandchild3
					sleep(2);
					waitpid(pid9, NULL, 0);
					exit(0);
				}

				exit(0);
			} else {
				pid_t pid10 = fork();
				if (pid10 == 0) {
					// Great Grandchild3.1
					x = set_weight(110);
					assert(x == 0);
					exit(0);
				} else {
					// Grandchild3
					sleep(2);
					waitpid(pid10, NULL, 0);
					waitpid(pid8, NULL, 0);
					exit(0);
				}
			}
		} else {
			// Child3
			pid_t pid11 = fork();
			if (pid11 == 0) {
				// GrandChild3.1
				x = set_weight(120);
				assert(x == 0);
				pid_t mypid = getpid();
				if(write(fd[1], &mypid, sizeof(mypid)) == -1) {
					perror("write");
					exit(1);
				}
				close(fd[1]);

				exit(0);
			} else {
				// Child3
				close(fd[1]);
				sleep(2);
				waitpid(pid11, NULL, 0);
				waitpid(pid7, NULL, 0);
				exit(0);
			}
		}
	}

	// wait for children to set weight
	sleep(1);
	close(fd[1]);
	if (read(fd[0], &heaviest_pid, sizeof(heaviest_pid)) == -1) {
		perror("read");
		exit(1);
	}
	close(fd[0]);
	heaviest = get_heaviest_descendant();
	assert(heaviest == heaviest_pid);

	// wait for children to finish
	waitpid(pid, NULL, 0);
	waitpid(pid4, NULL, 0);
	waitpid(pid6, NULL, 0);


	// Test multiple descendants with the same maximal weight - should be lowest pid

	if(pipe(fd) == -1) {
		perror("pipe");
		exit(1);
	}

	pid = fork();

	if (pid == 0) {
		// Child1
		close(fd[0]);
		x = set_weight(10);
		assert(x == 0);
		pid2 = fork();
		if (pid2 == 0) {
			// Grandchild1 
			x = set_weight(80);
			assert(x == 0);	
			pid3 = fork();
			if (pid3 == 0) {
				// Great Grandchild1
				x = set_weight(80);
				pid_t mypid = std::min(getpid(), getppid());
				if(write(fd[1], &mypid, sizeof(mypid)) == -1) {
					perror("write");
					exit(1);
				}
				close(fd[1]);
				assert(get_weight() == 80);
				exit(0);
			} else {
				// Grandchild1
				close(fd[1]);
				sleep(2);
				waitpid(pid3, NULL, 0);
				exit(0);
			}
			exit(0);
		} else {
			// Child1
			close(fd[1]);
			sleep(2);
			waitpid(pid2, NULL, 0);
			exit(0);
		}
	}

	pid4 = fork();

	if (pid4 == 0) {
		// Child2
		close(fd[0]);
		close(fd[1]);
		x = set_weight(60);
		assert(x == 0);
		pid2 = fork();
		if (pid2 == 0) {
			// Grandchild2
			x = set_weight(60);
			assert(get_weight() == 60);
			exit(0);
		} else {
			// Child2
			sleep(2);
			waitpid(pid2, NULL, 0);
			exit(0);
		}
	}

	// wait for children to set weight
	sleep(1);
	close(fd[1]);
	if (read(fd[0], &heaviest_pid, sizeof(heaviest_pid)) == -1) {
		perror("read");
		exit(1);
	}
	close(fd[0]);
	heaviest = get_heaviest_descendant();
	assert(heaviest == heaviest_pid);

	// wait for children to finish
	waitpid(pid, NULL, 0);
	waitpid(pid4, NULL, 0);


	cout << "===== PASSED HEAVIEST DESCENDANT TEST =====" << endl;


    cout << "===== SUCCESS =====" << endl;
    return 0;
}

