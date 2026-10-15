/*
 * Grocery Basket Planner
 * DAA Lab Mini Project
 *
 * You add grocery items (like vegetables) with their weight and how much
 * protein they give. You also set how much weight your basket can carry.
 * The program picks the items that give the most protein without going
 * over the basket limit.
 *
 * Algorithms used:
 *   Dynamic Programming  : 0/1 Knapsack  (best basket)
 *   Branch and Bound     : 0/1 Knapsack  (same answer, fewer steps)
 *   Greedy               : Fractional Knapsack (quick estimate)
 *   Divide and Conquer   : Merge Sort, Quick Sort, Binary Search
 *
 * Compile:  gcc grocery_planner.c -o grocery
 * Run:      ./grocery
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_ITEMS 50
#define MAX_WEIGHT 100000       // grams (100 kg)
#define MAX_PROTEIN 10000       // grams
#define BB_NODE_LIMIT 5000000
#define FILE_NAME "groceries.txt"

typedef struct {
    char name[30];
    int weight;     // in grams
    int protein;    // in grams
} Item;

Item items[MAX_ITEMS];
int count = 0;
int limit = 0;      // basket limit in grams

long ops = 0;       // counts comparisons / table updates for the analysis part


/* ---------- input / output helpers ---------- */

// reads one line, removes the newline, throws away anything that did not fit
void readLine(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL)
        exit(0);
    if (strchr(buf, '\n') == NULL) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
    buf[strcspn(buf, "\n")] = '\0';
}

int readInt(const char *msg, int lo, int hi)
{
    char buf[100], extra;
    int x;
    while (1) {
        printf("%s", msg);
        readLine(buf, sizeof(buf));
        // the %c catches leftover characters, so 3.5 or 12abc is rejected
        if (sscanf(buf, "%d %c", &x, &extra) == 1 && x >= lo && x <= hi)
            return x;
        printf("  Please enter a whole number from %d to %d.\n", lo, hi);
    }
}

// reads a weight like 500, 500g, 1.5kg or 2 kg. A plain number means grams.
// returns the weight in grams
int readWeight(const char *msg, int lo, int hi)
{
    char buf[100], unit[20], extra;
    double num, grams;
    int n, i;
    while (1) {
        printf("%s", msg);
        readLine(buf, sizeof(buf));
        unit[0] = '\0';
        n = sscanf(buf, "%lf %19s %c", &num, unit, &extra);
        if ((n == 1 || n == 2) && num > 0) {
            for (i = 0; unit[i] != '\0'; i++)
                unit[i] = tolower((unsigned char)unit[i]);
            grams = -1;
            if (unit[0] == '\0' || strcmp(unit, "g") == 0)
                grams = num;
            else if (strcmp(unit, "kg") == 0)
                grams = num * 1000;
            if (grams >= 0) {
                int g = (int)(grams + 0.5);
                if (g >= lo && g <= hi)
                    return g;
            }
        }
        printf("  Enter a weight like 500g or 1.5kg (between %d g and %d kg).\n", lo, hi / 1000);
    }
}

void readText(const char *msg, char *out, int size)
{
    printf("%s", msg);
    readLine(out, size);
}

// writes a weight in a friendly way: 500 g, 1.25 kg, 3 kg
void fmtWeight(int g, char *out)
{
    int len;
    if (g < 1000) {
        sprintf(out, "%d g", g);
        return;
    }
    sprintf(out, "%.3f", g / 1000.0);
    len = (int)strlen(out);
    while (len > 0 && out[len - 1] == '0')
        out[--len] = '\0';
    if (len > 0 && out[len - 1] == '.')
        out[--len] = '\0';
    strcat(out, " kg");
}

void pause()
{
    char buf[10];
    printf("\nPress Enter to go back to the menu...");
    readLine(buf, sizeof(buf));
}


/* ---------- file save / load ---------- */

void saveItems()
{
    FILE *f = fopen(FILE_NAME, "w");
    int i;
    if (f == NULL)
        return;
    fprintf(f, "%d\n", limit);
    for (i = 0; i < count; i++)
        fprintf(f, "%s|%d|%d\n", items[i].name, items[i].weight, items[i].protein);
    fclose(f);
}

void loadItems()
{
    FILE *f = fopen(FILE_NAME, "r");
    char line[100];
    if (f == NULL)
        return;
    if (fgets(line, sizeof(line), f)) {
        sscanf(line, "%d", &limit);
        if (limit < 0 || limit > MAX_WEIGHT)
            limit = 0;
    }
    while (count < MAX_ITEMS && fgets(line, sizeof(line), f)) {
        Item it;
        if (sscanf(line, "%29[^|]|%d|%d", it.name, &it.weight, &it.protein) == 3
            && it.weight >= 1 && it.weight <= MAX_WEIGHT
            && it.protein >= 0 && it.protein <= MAX_PROTEIN)
            items[count++] = it;
    }
    fclose(f);
}


/* ---------- basic item operations ---------- */

void addItem()
{
    Item it;
    if (count >= MAX_ITEMS) {
        printf("List is full (max %d items).\n", MAX_ITEMS);
        return;
    }
    readText("Item name: ", it.name, sizeof(it.name));
    if (strlen(it.name) == 0) {
        printf("Name cannot be empty.\n");
        return;
    }
    if (strchr(it.name, '|') != NULL) {
        printf("Name cannot contain the | character.\n");
        return;
    }
    it.weight = readWeight("Weight (like 500g or 1.5kg, plain number = grams): ", 1, MAX_WEIGHT);
    it.protein = readInt("Protein in grams (whole number): ", 0, MAX_PROTEIN);
    items[count++] = it;
    saveItems();
    printf("Added.\n");
}

void showItems()
{
    int i;
    char w[20];
    if (count == 0) {
        printf("No items yet.\n");
        return;
    }
    printf("\n%-4s %-22s %10s %11s %13s\n", "No.", "Item", "Weight", "Protein (g)", "Protein/100g");
    printf("----------------------------------------------------------------\n");
    for (i = 0; i < count; i++) {
        fmtWeight(items[i].weight, w);
        printf("%-4d %-22s %10s %11d %13.2f\n", i + 1, items[i].name, w,
               items[i].protein, (double)items[i].protein * 100 / items[i].weight);
    }
    printf("----------------------------------------------------------------\n");
    printf("Protein/100g = grams of protein you get from every 100 g of the item.\n");
    if (limit > 0) {
        fmtWeight(limit, w);
        printf("Basket limit: %s\n", w);
    } else {
        printf("Basket limit: not set\n");
    }
}

void deleteItem()
{
    int n, i;
    if (count == 0) {
        printf("Nothing to delete.\n");
        return;
    }
    showItems();
    n = readInt("\nItem number to delete (0 to cancel): ", 0, count);
    if (n == 0)
        return;
    for (i = n - 1; i < count - 1; i++)
        items[i] = items[i + 1];
    count--;
    saveItems();
    printf("Deleted.\n");
}

void setLimit()
{
    char w[20];
    limit = readWeight("Basket limit (like 3kg or 2500g, plain number = grams): ", 1, MAX_WEIGHT);
    saveItems();
    fmtWeight(limit, w);
    printf("Basket limit set to %s.\n", w);
}

void loadSample()
{
    const char *names[] = {"Potato", "Pumpkin", "Tomato", "Onion", "Spinach", "Cauliflower",
                           "Cabbage", "Carrot", "Green peas", "Broccoli", "Sweet corn",
                           "Paneer", "Tofu", "Peanuts", "Moong dal", "Soya chunks"};
    int weights[] = {1000, 1500, 500, 1000, 250, 600, 800, 500, 300, 400, 400, 200, 300, 250, 500, 200};
    int proteins[] = {20, 15, 5, 11, 7, 11, 10, 5, 15, 11, 13, 36, 24, 64, 120, 104};
    int n = sizeof(weights) / sizeof(weights[0]);
    int i;
    count = 0;
    for (i = 0; i < n; i++) {
        strcpy(items[i].name, names[i]);
        items[i].weight = weights[i];
        items[i].protein = proteins[i];
        count++;
    }
    limit = 3000;
    saveItems();
    printf("Loaded %d sample items, basket limit set to 3 kg.\n", n);
}


/* ---------- Divide and Conquer: sorting ---------- */

// compares two names ignoring upper/lower case
int nameCompare(const char *a, const char *b)
{
    while (*a && *b) {
        int x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x != y)
            return x - y;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

// by: 2 = weight, 3 = protein, 4 = protein per gram
double keyOf(Item a, int by)
{
    if (by == 2) return a.weight;
    if (by == 3) return a.protein;
    return (double)a.protein / a.weight;
}

// by: 1 = name, 2 = weight, 3 = protein, 4 = protein per 100 g
// returns 1 if a should come before b
int before(Item a, Item b, int by, int desc)
{
    ops++;
    if (by == 1) {
        int c = nameCompare(a.name, b.name);
        return desc ? c > 0 : c < 0;
    }
    if (desc)
        return keyOf(a, by) > keyOf(b, by);
    return keyOf(a, by) < keyOf(b, by);
}

void merge(Item a[], int l, int m, int r, int by, int desc)
{
    Item tmp[MAX_ITEMS];
    int i = l, j = m + 1, k = 0;
    while (i <= m && j <= r) {
        if (before(a[j], a[i], by, desc))
            tmp[k++] = a[j++];
        else
            tmp[k++] = a[i++];
    }
    while (i <= m) tmp[k++] = a[i++];
    while (j <= r) tmp[k++] = a[j++];
    for (i = 0; i < k; i++)
        a[l + i] = tmp[i];
}

void mergeSort(Item a[], int l, int r, int by, int desc)
{
    int m;
    if (l >= r)
        return;
    m = (l + r) / 2;
    mergeSort(a, l, m, by, desc);
    mergeSort(a, m + 1, r, by, desc);
    merge(a, l, m, r, by, desc);
}

int partition(Item a[], int l, int r, int by, int desc)
{
    Item pivot = a[r], t;
    int i = l - 1, j;
    for (j = l; j < r; j++) {
        if (before(a[j], pivot, by, desc)) {
            i++;
            t = a[i]; a[i] = a[j]; a[j] = t;
        }
    }
    t = a[i + 1]; a[i + 1] = a[r]; a[r] = t;
    return i + 1;
}

void quickSort(Item a[], int l, int r, int by, int desc)
{
    int p;
    if (l < r) {
        p = partition(a, l, r, by, desc);
        quickSort(a, l, p - 1, by, desc);
        quickSort(a, p + 1, r, by, desc);
    }
}

void sortMenu()
{
    int algo, by, order;
    if (count == 0) {
        printf("No items to sort.\n");
        return;
    }
    printf("\nSort using:\n  1. Merge sort\n  2. Quick sort\n");
    algo = readInt("Choice: ", 1, 2);
    printf("Sort by:\n  1. Name\n  2. Weight\n  3. Protein\n  4. Protein per 100 g\n");
    by = readInt("Choice: ", 1, 4);
    if (by == 1)
        printf("Order:\n  1. A to Z\n  2. Z to A\n");
    else
        printf("Order:\n  1. Low to high\n  2. High to low\n");
    order = readInt("Choice: ", 1, 2);

    ops = 0;
    if (algo == 1)
        mergeSort(items, 0, count - 1, by, order == 2);
    else
        quickSort(items, 0, count - 1, by, order == 2);

    printf("\nSorted. (%ld comparisons)\n", ops);
    showItems();
    saveItems();
}


/* ---------- Divide and Conquer: binary search ---------- */

// compares the first letters of the name with the search text (ignores case)
int cmpPrefix(const char *name, const char *key)
{
    int i;
    for (i = 0; key[i] != '\0'; i++) {
        int a = tolower((unsigned char)name[i]);
        int b = tolower((unsigned char)key[i]);
        if (a != b)
            return a - b;
    }
    return 0;
}

// the list must be sorted by name
int binarySearch(Item a[], int n, const char *key)
{
    int lo = 0, hi = n - 1, mid, c;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        ops++;
        c = cmpPrefix(a[mid].name, key);
        if (c == 0)
            return mid;
        else if (c < 0)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return -1;
}

void searchByName()
{
    Item copy[MAX_ITEMS];
    char key[30], w[20];
    int pos, i;
    if (count == 0) {
        printf("No items yet.\n");
        return;
    }
    readText("Search for an item (the start of the name is enough): ", key, sizeof(key));
    if (strlen(key) == 0) {
        printf("Type something to search for.\n");
        return;
    }

    // binary search needs sorted data, so sort a copy by name first
    memcpy(copy, items, sizeof(Item) * count);
    mergeSort(copy, 0, count - 1, 1, 0);

    ops = 0;
    pos = binarySearch(copy, count, key);
    if (pos == -1) {
        printf("No item starts with \"%s\". (%ld steps)\n", key, ops);
        return;
    }
    printf("Found (%ld steps):\n", ops);
    // other names with the same start sit right next to it
    i = pos;
    while (i > 0 && cmpPrefix(copy[i - 1].name, key) == 0)
        i--;
    while (i < count && cmpPrefix(copy[i].name, key) == 0) {
        fmtWeight(copy[i].weight, w);
        printf("  %-22s %10s   %d g protein\n", copy[i].name, w, copy[i].protein);
        i++;
    }
}


/* ---------- printing a basket (used by DP and Branch and Bound) ---------- */

void printPlan(int chosen[], int best)
{
    int i, total = 0;
    char w[20];
    for (i = 0; i < count; i++) {
        if (chosen[i]) {
            fmtWeight(items[i].weight, w);
            printf("  %-22s %10s   %d g protein\n", items[i].name, w, items[i].protein);
            total += items[i].weight;
        }
    }
    if (total == 0)
        printf("  Nothing fits in this basket.\n");
    fmtWeight(total, w);
    printf("\nTotal weight  : %s\n", w);
    fmtWeight(limit - total, w);
    printf("Space left    : %s\n", w);
    printf("Total protein : %d g\n", best);
}


/* ---------- Dynamic Programming: 0/1 Knapsack ---------- */

// returns best total protein; chosen[i] = 1 if item i goes in the basket
int knapsack01(Item a[], int n, int W, int chosen[])
{
    int i, w, best;
    int cols = W + 1;
    int *dp = (int *)calloc((size_t)(n + 1) * cols, sizeof(int));
    if (dp == NULL) {
        printf("Not enough memory.\n");
        return 0;
    }

    for (i = 1; i <= n; i++) {
        for (w = 0; w <= W; w++) {
            dp[i * cols + w] = dp[(i - 1) * cols + w];          // leave item i out
            if (a[i - 1].weight <= w) {
                int take = dp[(i - 1) * cols + (w - a[i - 1].weight)] + a[i - 1].protein;
                if (take > dp[i * cols + w])
                    dp[i * cols + w] = take;                    // put item i in
            }
            ops++;
        }
    }
    best = dp[n * cols + W];

    // go backwards to find which items were picked
    w = W;
    for (i = n; i >= 1; i--) {
        if (dp[i * cols + w] != dp[(i - 1) * cols + w]) {
            chosen[i - 1] = 1;
            w -= a[i - 1].weight;
        } else {
            chosen[i - 1] = 0;
        }
    }
    free(dp);
    return best;
}

void bestBasket()
{
    int chosen[MAX_ITEMS], best;
    char w[20];
    if (count == 0 || limit == 0) {
        printf("Add items and set a basket limit first.\n");
        return;
    }
    ops = 0;
    best = knapsack01(items, count, limit, chosen);

    fmtWeight(limit, w);
    printf("\nBest basket for a %s limit (Dynamic Programming):\n\n", w);
    printPlan(chosen, best);
    printf("(table updates: %ld)\n", ops);
}


/* ---------- Branch and Bound: 0/1 Knapsack ---------- */

/*
 * Idea: go through the items one by one (most protein per gram first).
 * For each item we have two branches: put it in the basket, or leave it out.
 * Before going deeper we work out an upper bound = the most protein we could
 * possibly still get (using the greedy fractional idea). If even that upper
 * bound cannot beat the best answer found so far, we cut that branch off.
 */

Item bbItems[MAX_ITEMS];        // items sorted by protein per gram
int bbN, bbCap, bbBest;
int bbCur[MAX_ITEMS];           // what we are trying right now
int bbBestSet[MAX_ITEMS];       // best choice found so far
long bbNodes, bbPruned;
int bbStopped;

// upper bound for the branch that starts at 'level'
double bbBound(int level, int weight, int protein)
{
    double b = protein;
    int left = bbCap - weight, i;
    for (i = level; i < bbN && left > 0; i++) {
        if (bbItems[i].weight <= left) {
            left -= bbItems[i].weight;
            b += bbItems[i].protein;
        } else {
            b += (double)left * bbItems[i].protein / bbItems[i].weight;
            left = 0;
        }
    }
    return b;
}

void bbSearch(int level, int weight, int protein)
{
    if (bbStopped)
        return;
    if (bbNodes >= BB_NODE_LIMIT) {
        bbStopped = 1;
        return;
    }
    bbNodes++;

    if (protein > bbBest) {
        bbBest = protein;
        memcpy(bbBestSet, bbCur, sizeof(bbCur));
    }
    if (level == bbN)
        return;

    // protein is a whole number, so we can round the bound down
    if ((int)(bbBound(level, weight, protein) + 0.000001) <= bbBest) {
        bbPruned++;
        return;
    }

    // branch 1: put this item in
    if (weight + bbItems[level].weight <= bbCap) {
        bbCur[level] = 1;
        bbSearch(level + 1, weight + bbItems[level].weight, protein + bbItems[level].protein);
        bbCur[level] = 0;
    }
    // branch 2: leave this item out
    bbSearch(level + 1, weight, protein);
}

// returns best total protein; chosen[i] = 1 if item i goes in the basket
int branchAndBound(Item a[], int n, int W, int chosen[])
{
    int order[MAX_ITEMS];
    int i, j, tmp;

    // sort item numbers by protein per gram, best first (insertion sort)
    for (i = 0; i < n; i++)
        order[i] = i;
    for (i = 1; i < n; i++) {
        tmp = order[i];
        j = i - 1;
        while (j >= 0 && (long long)a[tmp].protein * a[order[j]].weight >
                         (long long)a[order[j]].protein * a[tmp].weight) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = tmp;
    }
    for (i = 0; i < n; i++)
        bbItems[i] = a[order[i]];

    bbN = n;
    bbCap = W;
    bbBest = 0;
    bbNodes = 0;
    bbPruned = 0;
    bbStopped = 0;
    for (i = 0; i < n; i++) {
        bbCur[i] = 0;
        bbBestSet[i] = 0;
        chosen[i] = 0;
    }

    bbSearch(0, 0, 0);

    for (i = 0; i < n; i++)
        if (bbBestSet[i])
            chosen[order[i]] = 1;
    return bbBest;
}

void branchBoundPlan()
{
    int chosen[MAX_ITEMS], best;
    char w[20];
    if (count == 0 || limit == 0) {
        printf("Add items and set a basket limit first.\n");
        return;
    }
    best = branchAndBound(items, count, limit, chosen);

    fmtWeight(limit, w);
    printf("\nBest basket for a %s limit (Branch and Bound):\n\n", w);
    printPlan(chosen, best);
    printf("(branches tried: %ld, branches cut off: %ld)\n", bbNodes, bbPruned);
    if (bbStopped)
        printf("Stopped early (too many branches), so this may not be the best answer.\n");
}


/* ---------- Greedy: Fractional Knapsack ---------- */

double fractionalKnapsack(Item a[], int n, int W, int verbose)
{
    Item copy[MAX_ITEMS];
    double total = 0;
    int left = W, i;
    char w[20];

    memcpy(copy, a, sizeof(Item) * n);
    mergeSort(copy, 0, n - 1, 4, 1);      // most protein per gram first

    for (i = 0; i < n && left > 0; i++) {
        if (copy[i].weight <= left) {
            left -= copy[i].weight;
            total += copy[i].protein;
            if (verbose) {
                fmtWeight(copy[i].weight, w);
                printf("  %-22s full        %10s\n", copy[i].name, w);
            }
        } else {
            double frac = (double)left / copy[i].weight;
            total += frac * copy[i].protein;
            if (verbose) {
                fmtWeight(left, w);
                printf("  %-22s %5.1f%% only  %10s\n", copy[i].name, frac * 100, w);
            }
            left = 0;
        }
    }
    return total;
}

void quickPlan()
{
    double total;
    char w[20];
    if (count == 0 || limit == 0) {
        printf("Add items and set a basket limit first.\n");
        return;
    }
    fmtWeight(limit, w);
    printf("\nGreedy plan (most protein per gram first), basket limit %s:\n\n", w);
    total = fractionalKnapsack(items, count, limit, 1);
    printf("\nTotal protein : %.2f g\n", total);
    printf("\nNote: this assumes you can buy part of an item (like loose vegetables).\n");
    printf("For things you buy whole, use option 6 or 7 instead. This value is\n");
    printf("usually an upper limit of what those two can reach.\n");
}


/* ---------- compare the algorithms ---------- */

double timeInMicro(clock_t start, clock_t end, int runs)
{
    return ((double)(end - start) / CLOCKS_PER_SEC) * 1000000.0 / runs;
}

void compareAll()
{
    Item copy[MAX_ITEMS];
    int chosen[MAX_ITEMS];
    int runs, bbRuns, r, dpValue, bbValue;
    long cells, dpOps, bbSteps, mergeOps, quickOps;
    double fracValue, tDP, tBB, tFrac, tMerge, tQuick;
    clock_t t;
    char w[20];

    if (count == 0 || limit == 0) {
        printf("Add items and set a basket limit first.\n");
        return;
    }

    // knapsack: DP
    cells = (long)(count + 1) * (limit + 1);
    runs = (int)(50000000L / cells);
    if (runs < 1) runs = 1;
    if (runs > 2000) runs = 2000;

    ops = 0;
    dpValue = knapsack01(items, count, limit, chosen);
    dpOps = ops;
    t = clock();
    for (r = 0; r < runs; r++)
        knapsack01(items, count, limit, chosen);
    tDP = timeInMicro(t, clock(), runs);

    // knapsack: branch and bound
    bbValue = branchAndBound(items, count, limit, chosen);
    bbSteps = bbNodes;
    bbRuns = (int)(2000000L / (bbSteps + 1));
    if (bbRuns < 1) bbRuns = 1;
    if (bbRuns > 2000) bbRuns = 2000;
    t = clock();
    for (r = 0; r < bbRuns; r++)
        branchAndBound(items, count, limit, chosen);
    tBB = timeInMicro(t, clock(), bbRuns);

    // knapsack: greedy
    fracValue = fractionalKnapsack(items, count, limit, 0);
    t = clock();
    for (r = 0; r < 5000; r++)
        fractionalKnapsack(items, count, limit, 0);
    tFrac = timeInMicro(t, clock(), 5000);

    // sorting: merge vs quick (sorting by weight)
    memcpy(copy, items, sizeof(Item) * count);
    ops = 0;
    mergeSort(copy, 0, count - 1, 2, 0);
    mergeOps = ops;
    t = clock();
    for (r = 0; r < 5000; r++) {
        memcpy(copy, items, sizeof(Item) * count);
        mergeSort(copy, 0, count - 1, 2, 0);
    }
    tMerge = timeInMicro(t, clock(), 5000);

    memcpy(copy, items, sizeof(Item) * count);
    ops = 0;
    quickSort(copy, 0, count - 1, 2, 0);
    quickOps = ops;
    t = clock();
    for (r = 0; r < 5000; r++) {
        memcpy(copy, items, sizeof(Item) * count);
        quickSort(copy, 0, count - 1, 2, 0);
    }
    tQuick = timeInMicro(t, clock(), 5000);

    fmtWeight(limit, w);
    printf("\nInput: %d items, basket limit %s\n", count, w);

    printf("\nBest basket\n");
    printf("%-28s %12s %12s %12s\n", "Algorithm", "Protein (g)", "Steps", "Time (us)");
    printf("----------------------------------------------------------------\n");
    printf("%-28s %12d %12ld %12.2f\n", "0/1 Knapsack (DP)", dpValue, dpOps, tDP);
    printf("%-28s %12d %12ld %12.2f\n", "0/1 Knapsack (Branch&Bound)", bbValue, bbSteps, tBB);
    printf("%-28s %12.2f %12s %12.2f\n", "Fractional (Greedy)", fracValue, "-", tFrac);
    printf("(DP steps = table updates, Branch&Bound steps = branches tried)\n");

    printf("\nSorting by weight\n");
    printf("%-28s %12s %12s\n", "Algorithm", "Comparisons", "Time (us)");
    printf("----------------------------------------------------\n");
    printf("%-28s %12ld %12.2f\n", "Merge sort", mergeOps, tMerge);
    printf("%-28s %12ld %12.2f\n", "Quick sort", quickOps, tQuick);

    printf("\nTimes are averages over many runs, so they may vary a little.\n");
}


/* ---------- about page ---------- */

void about()
{
    printf("\nAlgorithms used in this project\n\n");
    printf("%-22s %-20s %-14s %s\n", "Algorithm", "Type", "Time", "Used for");
    printf("---------------------------------------------------------------------------\n");
    printf("%-22s %-20s %-14s %s\n", "0/1 Knapsack", "Dynamic Programming", "O(n*W)", "Best basket");
    printf("%-22s %-20s %-14s %s\n", "0/1 Knapsack", "Branch and Bound", "O(2^n) worst", "Best basket");
    printf("%-22s %-20s %-14s %s\n", "Fractional Knapsack", "Greedy", "O(n log n)", "Quick estimate");
    printf("%-22s %-20s %-14s %s\n", "Merge sort", "Divide and Conquer", "O(n log n)", "Sorting items");
    printf("%-22s %-20s %-14s %s\n", "Quick sort", "Divide and Conquer", "O(n log n) avg", "Sorting items");
    printf("%-22s %-20s %-14s %s\n", "Binary search", "Divide and Conquer", "O(log n)", "Find by name");
    printf("\nn = number of items, W = basket limit in grams\n");
    printf("Branch and Bound is slow in the worst case but usually tries far\n");
    printf("fewer branches than 2^n because it cuts off bad ones early.\n");
}


/* ---------- main ---------- */

void menu()
{
    char w[20];
    printf("\n==============================\n");
    printf("  GROCERY BASKET PLANNER\n");
    printf("==============================\n");
    if (limit > 0)
        fmtWeight(limit, w);
    else
        strcpy(w, "not set");
    printf("  Items: %d     Basket limit: %s\n", count, w);
    printf("------------------------------\n");
    printf("ITEMS\n");
    printf("   1. Add item\n");
    printf("   2. View items\n");
    printf("   3. Delete item\n");
    printf("   4. Set basket limit\n");
    printf("   5. Load sample items\n");
    printf("PLANS\n");
    printf("   6. Best basket (DP)\n");
    printf("   7. Best basket (Branch and Bound)\n");
    printf("   8. Quick plan (Greedy)\n");
    printf("TOOLS\n");
    printf("   9. Sort items\n");
    printf("  10. Search by name\n");
    printf("  11. Compare algorithms\n");
    printf("  12. About algorithms\n");
    printf("   0. Save and exit\n");
    printf("------------------------------\n");
}

int main()
{
    int choice;
    loadItems();

    while (1) {
        menu();
        choice = readInt("Enter choice: ", 0, 12);
        printf("\n");
        switch (choice) {
            case 1:  addItem();         break;
            case 2:  showItems();       break;
            case 3:  deleteItem();      break;
            case 4:  setLimit();        break;
            case 5:  loadSample();      break;
            case 6:  bestBasket();      break;
            case 7:  branchBoundPlan(); break;
            case 8:  quickPlan();       break;
            case 9:  sortMenu();        break;
            case 10: searchByName();    break;
            case 11: compareAll();      break;
            case 12: about();           break;
            case 0:
                saveItems();
                printf("Saved. Bye.\n");
                return 0;
        }
        pause();
    }
    return 0;
}
