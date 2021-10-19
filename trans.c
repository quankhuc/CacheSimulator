/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    /* For 32x32 matrix, we can loop in a block of 8x8 which is the size of the cache,
     * we can take care of diagonals by assigning each (when iterating through the for-loop)
     * into temporal_locality variable and have the diagonal idex stored in "diagonal"
     * Will have to write the code first to see if it can pass the optimzation test
     * Found out that for the matrix 32x32, it is not optimized yet. The optimized number of misses are 128
     */
    if(N == 32 && M == 32){
        // at here we block 8 at a time, because 8x8 is the size of the cache. For each iterations,
        // number of local variables allowed is 12
        int row_count;
        int column_count;
        int row_block;
        int column_block;
        // need local variables to store the diagonal to avoid conflict misses.
        // this variable holds the index of all diagonals in a block
        int diagonal = 0;
        // this variable stores a diagonal in cache
        int temporal_locality = 0;
        // we use the entire cache. If we access every row of a column, we can utilize the spatial locality that
        // there are only cold misses when we load the first element of each row
        for(column_block = 0; column_block < M; column_block += 8){
            for(row_block = 0; row_block < N; row_block += 8){
                // we took care of iterations to go through each block, now we loop within a block
                for(row_count = row_block; row_count < row_block + 8; ++row_count){
                    for(column_count = column_block; column_count < column_block + 8; ++column_count){
                        //as long as we are not on the diagonals, we can transpose a cell by taking 
                        //B[column][row] =  A[row][column] without conflict misses
                        if(row_count != column_count){
                            B[column_count][row_count] = A[row_count][column_count];
                        }
                        else{
                            //the diagonal variable stores the index of the diagonal
                            //diagonal is row_count, because after the first for-loop runs, diagonal is stored in set0 of the cache 
                            diagonal = row_count;
                            //temporal variable holds the value of the diagonal and is a hit because it takes the values from
                            //set1 of the cache and so on. For the value of the first diagonal, it is 0 which is the same value as
                            //the first value of set0 of the cache
                            temporal_locality = A[row_count][column_count];
                        }
                    }
                    //In a squared matrix like a 32x32 matrix, the value of the diagonal does not change.
                    //We have saved the value of the diagonal to a local variable so we can use that now.
                    if(column_block == row_block){
                        B[diagonal][diagonal] = temporal_locality;
                    }
                }
            }
        }
    }
    else if(M == 64 && N == 64){
        /* using a block 8x8 in matrix A then put into 4 blocks 4x4 within an 8x8 block
         * if I store the diagonal into local variables, that might solve the problem?
         * Solution: create 8 local variables to hold the 8 values in every row of a block in A.
         * Then half of those local variables are the first column of the new first 4x4 block in B.
         * The other half of those variables are the first column of the new second 4x4 block in B.
         * The index into the second half is "+4".
         * For the diagonal values, for example in the first block, the values are in this order
         * (0, 8, 16, 24) and repeats twice. It would be the same if I store them in these local variables
         * and have their values assigned from 2 columns next to another (which would be a cold miss first then a hit because they were loaded into
         * the cache)
         * Also can reuse i many times 
         */
        //I would need to create 8 local variables + 3 variables = 11 local variables < 12

        int column_block, row_block, i, local0, local1, local2, local3, local4, local5, local6, local7;

        for(column_block = 0; column_block < M; column_block += 8){
            for(row_block = 0; row_block < N; row_block += 8){
                for(i = 0; i < 4; ++i){
                    //This for-loop goes through the block and assign first 8 local variables
                    //and eaach time takes 8 datapoints of a row (total of 4 rows)
                    local0 = A[column_block + i][row_block + 0];
                    local1 = A[column_block + i][row_block + 1];
                    local2 = A[column_block + i][row_block + 2];
                    local3 = A[column_block + i][row_block + 3];
                    local4 = A[column_block + i][row_block + 4];
                    local5 = A[column_block + i][row_block + 5];
                    local6 = A[column_block + i][row_block + 6];
                    local7 = A[column_block + i][row_block + 7];

                    //assign first 4 local variables to be a column in the first block 4x4 in B
                    //others to be the a column in the second block 4x4 in B 
                    //NEED TO MAKE SURE THAT EACH ENTY IS TRANSPOSED CORRECTLY.
                    //I transposed in this order to follow the algorithm, which is to fill the  first column of the right first 4x4 block
                    // then to fill the first column of the left second 4x4 block. All these 4x4 blocks are in the same 4x8 block 
                    B[row_block + 0][column_block + i + 0] = local0; 
                    B[row_block + 0][column_block + i + 4] = local5; 
                   
                    B[row_block + 1][column_block + i + 0] = local1; 
                    B[row_block + 1][column_block + i + 4] = local6;
                   
                    B[row_block + 2][column_block + i + 0] = local2; 
                    B[row_block + 2][column_block + i + 4] = local7; 
                   
                    B[row_block + 3][column_block + i + 0] = local3; 
                    B[row_block + 3][column_block + i + 4] = local4;                      
                }
                //at the end of this for-loop, it should transpose a block of 4x8 in A
                //to a block of 4x8 in B
                /*The next row of the block 4x8 in B that is just right below the 4x8 recently transposed,
                 *has the same column. Therefore, in order to utilize locality, I can just use the values, 
                 *which are already loaded in.
                 */
                //fil in the first row of the second block to use temporial locality with local variables
                local0 = A[column_block + 4][row_block + 4];
                local1 = A[column_block + 5][row_block + 4];
                local2 = A[column_block + 6][row_block + 4];
                local3 = A[column_block + 7][row_block + 4];
                local4 = A[column_block + 4][row_block + 3];
                local5 = A[column_block + 5][row_block + 3];
                local6 = A[column_block + 6][row_block + 3];
                local7 = A[column_block + 7][row_block + 3];
                
                //this 3 lines use the first entry of the block and assign it to the first element of the first row
                //in the second 4x8 block. Then it assgined the proper diagonal value.
                //I also used these local variables to fill in the correct value of the remaining entries in a row

                B[row_block + 4][column_block + 0] = B[row_block + 3][column_block + 4];
                B[row_block + 4][column_block + 4] = local0;
                B[row_block + 3][column_block + 4] = local4;
              
                B[row_block + 4][column_block + 1] = B[row_block + 3][column_block + 5];
                B[row_block + 4][column_block + 5] = local1;
                B[row_block + 3][column_block + 5] = local5;

                B[row_block + 4][column_block + 2] = B[row_block + 3][column_block + 6];
                B[row_block + 4][column_block + 6] = local2;
                B[row_block + 3][column_block + 6] = local6;
              
                B[row_block + 4][column_block + 3] = B[row_block + 3][column_block + 7];
                B[row_block + 4][column_block + 7] = local3;
                B[row_block + 3][column_block + 7] = local7;

                //only need to iterate 3 times because first row of the block is 
                //already filled to use for temporal locality
                for(i = 0; i < 3; ++i){
                    local0 = A[column_block + 4][row_block + 5 + i];
                    local1 = A[column_block + 5][row_block + 5 + i];
                    local2 = A[column_block + 6][row_block + 5 + i];
                    local3 = A[column_block + 7][row_block + 5 + i];
                    local4 = A[column_block + 4][row_block + i];
                    local5 = A[column_block + 5][row_block + i];
                    local6 = A[column_block + 6][row_block + i];
                    local7 = A[column_block + 7][row_block + i];
                
                    //fill in the remaining rows of the block 4x8 
                    B[row_block + 5 + i][column_block + 0] = B[row_block + i][column_block + 4];
                    B[row_block + 5 + i][column_block + 4] = local0;
                    B[row_block + i][column_block + 4] = local4;

                    B[row_block + 5 + i][column_block + 1] = B[row_block + i][column_block + 5];
                    B[row_block + 5 + i][column_block + 5] = local1;
                    B[row_block + i][column_block + 5] = local5;

                    B[row_block + 5 + i][column_block + 2] = B[row_block + i][column_block + 6];
                    B[row_block + 5 + i][column_block + 6] = local2;
                    B[row_block + i][column_block + 6] = local6;
                    
                    B[row_block + 5 + i][column_block + 3] = B[row_block + i][column_block + 7];
                    B[row_block + 5 + i][column_block + 7] = local3;
                    B[row_block + i][column_block + 7] = local7;

                } 
            }
        }
    }
    else if(M == 61 && N == 67){
        // at here we block 8 at a time, because 8x8 is the size of the cache. For each iterations,
        // number of local variables allowed is 12
        int row_count;
        int column_count;
        int row_block;
        int column_block;
        // need local variables to store the diagonal to avoid conflict misses.
        // this variable holds the index of all diagonals in a block
        int diagonal = 0;
        // this variable stores a diagonal in cache
        int temporal_locality = 0;
        // we use the entire cache. If we access every row of a column, we can utilize the spatial locality that
        // there are only cold misses when we load the first element of each row
        for(column_block = 0; column_block < M; column_block += 8){
            for(row_block = 0; row_block < N; row_block += 8){
                // we took care of iterations to go through each block, now we loop within a block
                // I have to include another condition to check for the validity of blocking because there would be parts of
                // the matrix that can't be blocked. For example, when row_block + 8 is bigger than 67, it can't be blocked because
                // there are only 61 rows.
                for(row_count = row_block; (row_count < N) && ( row_count < row_block + 8); ++row_count){
                    for(column_count = column_block; (column_count < M) && ( column_count < column_block + 8); ++column_count){
                        //as long as we are not on the diagonals, we can transpose a cell by taking 
                        //B[column][row] =  A[row][column] without conflict misses
                        if(row_count != column_count){
                            B[column_count][row_count] = A[row_count][column_count];
                        }
                        else{
                            //the diagonal variable stores the index of the diagonal
                            //diagonal is row_count, because after the first for-loop runs, diagonal is stored in set0 of the cache 
                            diagonal = row_count;
                            //temporal variable holds the value of the diagonal and is a hit because it takes the values from
                            //set1 of the cache and so on. For the value of the first diagonal, it is 0 which is the same value as
                            //the first value of set0 of the cache
                            temporal_locality = A[row_count][column_count];
                        }
                    }
                    //In a squared matrix like a 32x32 matrix, the value of the diagonal does not change.
                    //We have saved the value of the diagonal to a local variable so we can use that now.
                    if(column_block == row_block){
                        B[diagonal][diagonal] = temporal_locality;
                    }
                }
            }
        }
        
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

