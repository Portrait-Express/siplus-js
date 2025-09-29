const { siplus_init, SIPlus } = require("../dist/index.js");

test("Parser", async () => {
    await siplus_init();
    var parser = new SIPlus();

    var retriever = parser.parse_interpolation("Hello {.id}");

    expect(retriever.construct({id: 1})).toEqual("Hello 1");

    var retriever = parser.parse_expression("map .id");
    var value = retriever.retrieve([{ id: 1 }, { id: 2 }]);

    retriever.delete();
    parser.delete();

    expect(value).toEqual([1, 2])
})
