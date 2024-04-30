# Terminology

Iskierka codes often forms similar patterns that have been named.


## Root

This is the *output* variable. We start generating strings from here.

```
#output
greet
print('hello world')
```

## Leaf

Leaf is a variable that does not contain references to variables.

```
#int
two
2

#int
three
3

#int
five
5
```

## Flag

This variable can be skipped entirely or it inserts two values.

```
#warnings weight 5
##empty
##empty

#warnings weight 3
with warnings
| FLAG_SHOW_WARNINGS
```

## Decoration

This variable can be skipped entirely and it never affects the programming code output. An optionary decoration.

```
#optionaryOf weight 8
##empty
##empty

#optionaryOf weight 2
of
##empty
```

## Fractal

A variable that contains a reference to itself.
They are unstable and dangerous. Can potentially cause deep recursions and stack overflow.
See: Lindenmayer system.

```
#number
one
1

#number
two plus _number
2 + _number
```

## Synonym

A set of natural language texts that share similar meaning.

```
#selectionVerb
select
##empty

#selectionVerb
give
##empty

#selectionVerb
show
##empty
```

## Fork

Fork is a variable that chooses one variable out of few possibilities.

```
#numberOfItems weight 4
_positiveInt0to100
_positiveInt0to100

#numberOfItems weight 3
_positiveInt101to1000
_positiveInt101to1000

#numberOfItems weight 2
_positiveIntOver1000
_positiveIntOver1000
```

## Clone

Create another independent instance of a variable.

```
#int2
_int
_int

#output
add _int to _int2
print(str(_int + _int2))
```

## Glue

Concatenate strings seamlessly without spaces between them. Will produce *fiftyandfifty* and *50and50*.

```
#int
fifty
50

#and
and
and

#output
_int_and_int
_int_and_int
```

## Mixer

This pattern produces all possible permutations of some flags.

```
#output
run this program _mixer
program.exe _mixer


#mixer
_f1 _f2 _f3
_f1 _f2 _f3

#mixer
_f1 _f3 _f2
_f1 _f3 _f2

#mixer
_f2 _f1 _f3
_f2 _f1 _f3

#mixer
_f2 _f3 _f1
_f2 _f3 _f1

#mixer
_f3 _f1 _f2
_f3 _f1 _f2

#mixer
_f3 _f2 _f1
_f3 _f2 _f1


#f1
with dynamic execution
-d

#f2
silently
-s

#f3
optimized
-o


#f1 weight 3
##empty
##empty

#f2 weight 2
##empty
##empty

#f3 weight 3
##empty
##empty
```

