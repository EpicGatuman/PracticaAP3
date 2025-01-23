Approach to solve the 2D strip-packing problem through:
- an exhaustive search approach (good for optimal solutions in small inputs) - execution needs to be stopped for large inputs (if it stops bf it's reached the optimal solution)
- a metaheuristic algorithm (good for any type of input, not always optimal) - execution needs to be stopped
- greedy algorithm (good to search a fast solution to a big input) - immediate execution

  EXECUTION:

Input file template:
3 3 (width of the table , number of shapes)
3  1 1  (number of shapes with that specific features, width, height)

Output file:
Height
Position of each shape (left top coordinate, right bottom coordinate)
