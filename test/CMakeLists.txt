file(GLOB_RECURSE TEST_SRC
    "*.cpp"
)
target_sources(${TEST_NAME} PRIVATE ${TEST_SRC})