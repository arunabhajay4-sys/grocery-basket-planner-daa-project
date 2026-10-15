# Grocery Basket Planner

DAA (Design and Analysis of Algorithms) lab mini project, written in C with a matching browser front end.

You have a basket that can carry only a fixed weight. Every grocery item has a **weight** and a **protein** amount. The program picks the items that give the **most protein without going over the weight limit**. That is the knapsack problem:

| Knapsack term | In this project        |
| ------------- | ---------------------- |
| Capacity      | Basket weight limit    |
| Item weight   | Weight of the grocery  |
| Item value    | Protein in grams       |

## Algorithms

| Algorithm           | Paradigm            | Time            | Used for                              |
| ------------------- | ------------------- | --------------- | ------------------------------------- |
| 0/1 Knapsack        | Dynamic Programming | O(n * W)        | Best basket (items taken whole)       |
| 0/1 Knapsack        | Branch and Bound    | O(2^n) worst    | Best basket, usually far fewer steps  |
| Fractional Knapsack | Greedy              | O(n log n)      | Loose items (vegetables, grains)      |
| Merge sort          | Divide and Conquer  | O(n log n)      | Sorting the item list                 |
| Quick sort          | Divide and Conquer  | O(n log n) avg  | Sorting the item list                 |
| Binary search       | Divide and Conquer  | O(log n)        | Finding an item by name               |

n = number of items, W = basket limit in grams.

## Files

| File                | What it is                                                     |
| ------------------- | -------------------------------------------------------------- |
| `grocery_planner.c` | Main program. Menu-driven console version in C.                |
| `grocery.html`      | Browser front end with the same features (double-click to open). |

The web page is a visual version of the same algorithms written in JavaScript. The C file is the main implementation.

## Run

C version:

```
gcc grocery_planner.c -o grocery
./grocery
```

On Windows with MinGW: `gcc grocery_planner.c -o grocery.exe` and then `grocery.exe`.

Web version: open `grocery.html` in any browser. No install or server needed.

## How to use

1. Set the basket limit (menu option 4).
2. Add items (option 1), or load sample groceries (option 5).
3. Pick a plan (options 6 to 8).

Weights can be typed as `250`, `250g` or `1.5kg`. A plain number means grams. Protein is a whole number of grams for the whole item.

### Menu

| No. | Option                          | Algorithm            |
| --- | ------------------------------- | -------------------- |
| 1   | Add item                        |                      |
| 2   | View items                      |                      |
| 3   | Delete item                     |                      |
| 4   | Set basket limit                |                      |
| 5   | Load sample items               |                      |
| 6   | Best basket (DP)                | 0/1 knapsack, DP     |
| 7   | Best basket (Branch and Bound)  | 0/1 knapsack, B&B    |
| 8   | Quick plan (Greedy)             | Fractional knapsack  |
| 9   | Sort items (name, weight, protein, protein per 100 g) | Merge sort / Quick sort |
| 10  | Search by name                  | Binary search        |
| 11  | Compare algorithms              | Steps and time       |
| 12  | About algorithms                |                      |
| 0   | Save and exit                   |                      |

The item table also shows **Protein/100g** = protein / weight x 100, which is the grams of protein you get from every 100 g of that item. Greedy and Branch and Bound rank items by the exact protein / weight ratio.

## Sample run

The sample data has 16 items and a 3 kg basket limit.

| Algorithm                   | Total protein | Steps                    |
| --------------------------- | ------------- | ------------------------ |
| 0/1 Knapsack (DP)           | 394 g         | 48,016 table updates     |
| 0/1 Knapsack (Branch&Bound) | 394 g         | 31 branches tried        |
| Fractional (Greedy)         | 398 g         | no table or branches     |

Things worth noticing:

- DP and Branch and Bound always reach the same total protein. When two different baskets tie, they can pick different items. In the sample, DP fills the basket to exactly 3 kg and Branch and Bound uses 2.8 kg, with the same 394 g of protein.
- Branch and Bound tried 31 branches instead of the 2^16 possible choices, because it cuts off branches whose best possible protein cannot beat the best basket found so far.
- Greedy scores higher (398 g) because it is allowed to take part of an item (20% of the potatoes). That makes it an upper limit for the 0/1 versions, which must take items whole. It is the realistic plan for loose vegetables and grains.

## Notes and limits

- Up to 50 items. One item and the basket can weigh up to 100 kg. Protein per item is up to 10,000 g.
- Items and the basket limit are saved in `groceries.txt` next to the program and loaded on the next start. The web version saves in the browser.
- The sample protein values are approximate. Edit them to match the food labels you use.
- Branch and Bound stops after 5,000,000 branches and says so, as a safety limit.
- Binary search needs sorted data, so the program sorts a copy of the list by name first. Typing the start of a name is enough (`pa` finds Paneer).
- Times in "Compare algorithms" are averages over many runs and change a little each time.
