### R-Trees: Efficient Spatial Indexing

R-trees are a fundamental data structure for efficiently indexing multi-dimensional information, such as geographical coordinates and spatial shapes. By organizing data in a hierarchical, tree-like structure, R-trees optimize storage and querying for a wide range of applications, including geographic information systems (GIS), spatial databases, and many more. This structure is adept at handling various spatial operations, significantly improving the performance of spatial queries and data retrieval.

### Project Implementation: Building an R-Tree from Scratch

This project presents a detailed implementation of an R-tree, focusing on its insertion mechanism. It covers key operations such as node splitting, adjustments of minimum bounding rectangles (MBRs), and ensuring the tree's balance, crucial for enhancing spatial querying efficiency. The implementation showcases the sequential insertion of data points into the R-tree from a given dataset, demonstrating the R-tree's dynamic structure adaptation to efficiently index spatial data.

### Project Organization and Visualization Tools

- **`code/rtree.c`**: This file is the backbone of the project, housing the implementation of the R-tree. It encompasses crucial functionalities such as the logic for inserting spatial data, splitting nodes when they exceed capacity, adjusting minimum bounding rectangles (MBRs) for optimal spatial encapsulation, and maintaining the tree's balance to ensure efficient query performance.

- **`visualizer/plot.ipyb`**: A Jupyter Notebook designed for visualizing the structure of the R-tree. It takes spatial data points from `data.txt` and generates visual representations of the R-tree at various stages of its construction, providing a clear picture of how the tree organizes and indexes spatial information through its hierarchical structure.

- **`code/myscript.sh`**: Executing `sh myscript.sh` enables you to use the RTree on your test cases.

#### Visualization Example

![image](https://github.com/risingPhoenix7/R-Tree-Guttman/assets/96655704/08d7e809-8886-40fd-9db7-34c3c665a5b3)


The visualization highlights the R-tree's hierarchical organization after several insertions. It demonstrates the R-tree's efficiency in spatial indexing, showcasing its capacity to optimize spatial queries and data retrieval through its structured spatial organization.