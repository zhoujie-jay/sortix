/*******************************************************************************

    Copyright(C) Steve Dougherty 2011.
    Copyright(C) Jonas 'Sortie' Termansen 2011.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    calc.cpp
    A simple reverse polish calculator.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//Stack, pending standard library implementation. All in one file due to linking
// annoyances with the makefile.
template <typename T>
class Stack {
	public:

	//Is its own iterator.
	/* MODIFIES: this.
	 * EFFECTS: Self-iterator: sets to first element and returns it.
	 */
	const T *begin() {
		currentIter = head;
		return next();
	}

	/* MODIFIES: this.
	 * EFFECTS: Self-iterator: returns element from the list and advances.
	 *          If all elements have been iterated, returns NULL. Does not
	 *          have invalidation - be careful!
	 */
	const T *next() {
		if (!currentIter) return NULL;

		const T *element = currentIter->o;
		currentIter = currentIter->next;

		return element;
	}

	/* EFFECTS: Returns true if stack is empty, false otherwise.
	 */
	bool isEmpty() const {
		return !head;
	}

	/* MODIFIES: this.
	 * EFFECTS: Pushes o onto the top of the stack.
	 */
	void push(T *o) {
		head = new node(head, o);
	}

	/* MODIFIES this
	 * EFFECTS Pops and returns the top element. Returns NULL if empty.
	 */
	T *pop() {
		if (isEmpty()) return NULL;

		node *oldHead = head;
		T *element = oldHead->o;
		head = oldHead->next;
		delete oldHead;

		return element;
	}

	Stack() : head(NULL) , currentIter(NULL) {}

	Stack(const Stack &s) : head(NULL) , currentIter(NULL) {
		s.begin();
		copyAll(s.head);
	}

	Stack &operator=(const Stack &s) {
		if (this == &s) return;
		removeAll();
		copyAll(s.head);
	}

	~Stack() {
		removeAll();
	}

	private:

	struct node {
		node(node *next, T *o) : next(next), o(o) {}
		node   *next;
		T      *o;
	};

	node *head;
	node *currentIter;

	/* MODIFIES: this.
	 * EFFECT: called by copy constructor and operator= to copy elements
	 * following from the given node into the local instance.
	 */
	void copyAll(const node *n) {
		if (n) {
			copyAll(n->next);
			push(new T(*(n->o)));
		}
	}

	void removeAll() {
		while (!isEmpty()) delete pop();
	}
};

/* MODIFIES: stack.
 * EFFECTS: Pushes value onto the stack.
 */
void push(long *value, Stack<long> &stack) {
	stack.push(value);
}

/* MODIFIES: stack.
 * EFFECTS: Pushes value onto the stack.
 */
void push(long value, Stack<long> &stack) {
	stack.push(new long(value));
}

/* MODIFIES: operand, stack, standard output.
 * EFFECTS: Attempts to set operand to the top value on the stack. If there
 *          are no operands on the stack, prints an error message and aborts
 *          the process with exit code 1.
 */
void pop(long *&operand, Stack<long> &stack) {
	if (stack.isEmpty()) {
		printf("%s: not enough operands\n", program_invocation_name);
		exit(1);
	}

	operand = stack.pop();
}

/* MODIFIES: operand1, operand2, stack, standard output.
 * EFFECTS: Attempts to remove two operands from the top of the stack, setting
 *          operand1 to the first one removed and operand2 to the second. If
 *          there are fewer than two enough operands on the stack, prints
 *          an error message and aborts the process with exit code 1.
 */
void popTwo(long *&operand1, long *&operand2, Stack<long> &stack) {
	pop(operand1, stack);
	pop(operand2, stack);
}

/* MODIFIES: stack, standard output.
 * EFFECTS: Attempts to replace the top two operands on the stack with the
 *          result returned by func(first, second).
 */
//Lambda functions would be wonderful for calling this function.
//http://www2.research.att.com/~bs/C++0xFAQ.html#lambda
void apply(long(*func)(long, long), Stack<long> &stack) {
	long *operand1, *operand2;
	popTwo(operand1, operand2, stack);
	push(func(*operand1, *operand2), stack);
	delete operand1;
	delete operand2;
}

/* EFFECTS: Returns first + second.
 */
long add(long first, long second) { return first + second; }

/* EFFECTS: Returns subtracting "the first number from the second" (pg. 5)
 */
long subtract(long first, long second) { return second - first; }

/* EFFECTS: Returns first * second.
 */
long multiply(long first, long second) { return first * second; }

/* MODIFIES: stack, standard output.
 * EFFECTS: Attempts to replace the top two operands on the stack with the
 *          result of dividing the second one by the first.
 *          If the first number is zero, prints an division by zero error and
 *          exits the process with exit code 1.
 */
void divide(Stack<long> &stack) {
	/* Can't use apply for this because it requires a check for the
	 * denominator being zero. It could have an error-checking function
	 * pointer, but that would muddy things as no other operators have such
	 * conditions.
	 */
	long *first, *second;

	popTwo(first, second, stack);
	if (*first == 0) {
		printf("%s: division by zero\n", program_invocation_name);
		exit(1);
	}

	push(*second / *first, stack);
	delete first;
	delete second;
}

/* MODIFIES: stack, standard output.
 * EFFECTS: Attempts to negate the top element on the stack.
 */
void negate(Stack<long> &stack) {
	long *operand;

	pop(operand, stack);
	push(-(*operand), stack);
	delete operand;
}

/* MODIFIES: stack, standard output.
 * EFFECTS: Attempts to push a copy of the top element on top of the already
 *          existing one.
 */
void duplicate(Stack<long> &stack) {
	long *operand;

	pop(operand, stack);
	//Dereferencing creates new instance.
	push(*operand, stack);
	push(operand, stack);
}

/* MODIFIES: stack, standard output.
 * EFFECTS: Attempts to reverse the first two elements on the stack.
 */
void reverse(Stack<long> &stack) {
	long *first, *second;

	popTwo(first, second, stack);
	//Push the former first before the former second.
	push(first, stack);
	push(second, stack);
}

/* MODIFIES: standard output.
 * EFFECTS: Prints the first element on the stack to standard output, followed
 *          by a newline.
 */
void print(Stack<long> &stack) {
	long *operand;

	pop(operand, stack);
	printf("%li\n", *operand);
	push(operand, stack);
}

/* MODIFIES: stack.
 * EFFECTS: Empties the stack.
 */
void clear(Stack<long> &stack) {
	while (!stack.isEmpty()) delete stack.pop();
}

/* MODIFIES: standard output.
 * EFFECTS: Prints all the elements on the stack, from top to bottom, each
 *          separated by a single space. No trailing space. Ends with newline.
 */
void printAll(Stack<long> &stack) {
	if (!stack.isEmpty()) {
		const long *element = stack.begin();
		do {
			printf("%li", *element);
		} while ((element = stack.next()) && printf(" "));
	}
	printf("\n");
}

/* MODIFIES: standard output.
 * EFFECTS: Prints usage information and returns 0.
 */
int printUsage(const char* argv0) {
	printf("Usage: %s [--help] [--usage] COMMAND ...\n", argv0);
	printf("%s is a Reverse Polish Notation integer calculator.\n", argv0);
	printf("\n");
	printf("  --help, --usage    Display this help and exit\n");
	printf("\n");
	printf("%s supports the following commands:\n", argv0);
	printf("Any integers supplied are pushed onto the stack and can be given in decimal\n");
	printf("(default), octal with a leading 0, or hexadecimal with a leading 0x. Numbers\n");
	printf("can be prefixed with + or - to specify positive or negative, respectively.\n");
	printf("The operators +, -, *, and / pop the top two elements and push the result of\n");
	printf("<second> <operator> <first> to the stack. The top element is printed on exit\n");
	printf("unless a print command is given or an error occurs. Also,\n");
	printf(" a: prints all elements from first to last.\n");
	printf(" c: clears the stack.\n");
	printf(" d: duplicates the top element.\n");
	printf(" n: negates the top element.\n");
	printf(" p: prints the top element.\n");
	printf(" r: reverses the order of the top two elements.\n");
	printf("\n");
	printf("Examples:\n");
	printf("  %s 2 2 +\n", argv0);
	printf("  %s 0xCAFE42 0777 *\n", argv0);
	return 0;
}

int main(int argc, char *argv[]) {
	//If no arguments, --usage, or --help.
	if (argc == 1 ||
	    (argc > 1 && (!strcmp(argv[1], "--usage") ||
	                  !strcmp(argv[1], "--help"))))
		return printUsage(argv[0]);

	Stack<long> stack;
	bool printedAnything = false;

	//Skip program name in arguments list.
	for (int i = 1; i < argc; i++) {
		char *endPtr;
		//Allow input in decimal, octal, or hex.
		long numberInput = strtol(argv[i], &endPtr, 0);

		//Make sure the string is a number and not empty.
		if (*argv[i] && !(*endPtr)) {
			push(numberInput, stack);
			continue;
		}

		/* Input was not a valid number, attempt to parse as operator.
		 * Anything other than one character is not a valid operator.
		 */
		if (strlen(argv[i]) == 1) {
			switch (argv[i][0]) {
				case '+':
					//Add
					apply(add, stack);
					continue;
				case '-':
					//Subtract
					apply(subtract, stack);
					continue;
				case '*':
					//Multiply
					apply(multiply, stack);
					continue;
				case '/':
					//Divide
					divide(stack);
					continue;
				case 'n':
					//Negate
					negate(stack);
					continue;
				case 'd':
					//Duplicate
					duplicate(stack);
					continue;
				case 'r':
					//Reverse
					reverse(stack);
					continue;
				case 'p':
					//Print
					print(stack);
					printedAnything = true;
					continue;
				case 'c':
					//Clear
					clear(stack);
					continue;
				case 'a':
					//Print-all
					printAll(stack);
					printedAnything = true;
					continue;
			}
		}

		printf("%s: unsupported command: %s\n", program_invocation_name, argv[i]);
		exit(1);
	}

	if ( !printedAnything && !stack.isEmpty() ) {
		print(stack);
	}

	return 0;
}
