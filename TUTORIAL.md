
# IskierkaGen quick tutorial

Iskierka code consists of *hash expressions*.
Each of them is a series of three lines of text.

```
#output
greet
print('hello, world')
```

The first one starts with *#* and is followed by a variable name.
The second line contains natural language text.
The third line is a code written in some programming language.


Every codebase needs at least one hash expression starting with variable *output*.

```
#output
exit
Application.close(0);
```

Each hash expression associates some natural language text with some programming language code.
When we run IskierkaGen with the code above and ask it to generate new values, the output will be always the same.
Value *exit* as natural language and *Application.close(0);* as code.

```
#output
give me three normal tickets
issueTickets(3, TicketType.Normal);

#output
give me dozen of normal tickets
issueTickets(12, TicketType.Normal);
```

This code contains two hash expressions with the *output* variable.
Now, there are two possibilities and the output is random. 
50% of the times it returns *give me three normal tickets* and *issueTickets(3, TicketType.Normal);*.
Otherwise it returns *give me dozen of normal tickets* and *issueTickets(12, TicketType.Normal);*.

```
#output
give me _ticketsNumber normal tickets
issueTickets(_ticketsNumber, TicketType.Normal);

#ticketsNumber
three
3

#ticketsNumber
dozen of
12
```

We can define new variables and use them as macros in code.
Code has been refactored and works exactly the same as previously.
Values of *_ticketsNumber* are picked randomly at runtime and are inserted into the output hash expression pattern.

```
#output weight 6
_ticketsNumber tickets please
issueTickets(_ticketsNumber, TicketType.Normal);

#output weight 3
give me _ticketsNumber tickets
issueTickets(_ticketsNumber, TicketType.Normal);

#output
give me _ticketsNumber normal tickets
issueTickets(_ticketsNumber, TicketType.Normal);

#ticketsNumber
three
3

#ticketsNumber
four
4

#ticketsNumber
dozen of
12

#ticketsNumber
pair of
2
```

Hash expressions can be weighted to shape a desired probability distribution. By default their weight is 1.
Only nonnegative integers allowed!
In this code, the first output pattern is the most important. 
It will be picked 60% of the times. The second is picked 30% of the times and the third with the default weight of 1 is used rarely (10%).
The 25% probability is used for every hash expression of *ticketsNumber*.


Special value *##empty* represents an empty string. 
It can be used to define some optionary natural language insertions that do not affect the programming language output code.

```
#optionaryPlease weight 2
please
##empty

#optionaryPlease weight 8
##empty
##empty
```

This is the entire language.
