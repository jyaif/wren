// Sprinkle some carriage returns to ensure they are ignored anywhere:
IO
IO.print

// Generate a compile error so we can test that the line number is correct.
+ * // expect error