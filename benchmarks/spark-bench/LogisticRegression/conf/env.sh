## Application parameters#32G date size=400 million examples; 1G= 12.5

############## 18GB #######################
# NUM_OF_EXAMPLES=20000
# NUM_OF_FEATURES=50000
###########################################

############## 32GB #######################
# NUM_OF_EXAMPLES=20000
# NUM_OF_FEATURES=88888
# NUM_OF_PARTITIONS=72
###########################################

############## 64GB #######################
NUM_OF_EXAMPLES=20000
NUM_OF_FEATURES=177777
#NUM_OF_PARTITIONS=128
NUM_OF_PARTITIONS=512
###########################################

ProbOne=0.2
EPS=0.5

MAX_ITERATION=100
SPARK_STORAGE_MEMORYFRACTION=0.9