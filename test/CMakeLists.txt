file(GLOB_RECURSE TEST_SRC
    "main.cpp"
    "test*.cpp"
)
target_sources(${TEST_NAME} PRIVATE ${TEST_SRC})