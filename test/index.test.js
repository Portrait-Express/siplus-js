const { siplus } = require("../dist/index.js");

function test_interpolation(parser, expr, value, expected) {
    var constructor = parser.parse_interpolation(expr);
    try {
        const result = constructor.construct(value);
        expect(result).toEqual(expected);
    } finally {
        constructor.delete();
    }
}

function test_expression(parser, expr, value, expected) {
    var retriever = parser.parse_expression(expr);
    try {
        const result = retriever.retrieve(value);
        expect(result).toEqual(expected);
    } finally {
        retriever.delete();
    }
}

describe('SIPlus Tests', () => {
    test("Parser", async () => {
        var parser = await siplus();

        test_interpolation(parser, "Hello { .id }", {id: 1}, "Hello 1")
        test_expression(parser, "map . .id", [{id: 1}, {id: 2}], [1, 2])

        parser.delete();
    })

    test("Converters", async () => {
        var parser = await siplus();

        test_expression(parser, `eq 9 .`,     "9",  true)
        test_expression(parser, `eq "9" .`,   9,    true)
        test_expression(parser, `and true .`,  true, true)
        test_expression(parser, `and false .`, true, false)

        parser.delete();
    })

    test("Function", async () => {
        var parser = await siplus();

        var func = (_, parent, toAppend) => {
            if(typeof parent !== "string" || typeof toAppend !== "string") {
                throw Error("Parent or toAppend is not string");
            }

            return parent + toAppend;
        }

        let ctx = parser.context()
        ctx.emplace_function("testAppend", func);
        ctx.delete();

        var retriever = parser.parse_expression(`"Hello, " | testAppend "World"`);
        expect(retriever.retrieve(null)).toEqual("Hello, World");

        retriever.delete();
        parser.delete();
    })

    test("Stdlib", async () => {
        var parser = await siplus();
        
        try {
            var retriever = parser.parse_expression(`"1,2,3,4,5" | split ","`);
            expect(retriever.retrieve(null)).toEqual(["1", "2", "3", "4", "5"]);
            retriever.delete();
            var retriever = parser.parse_expression(`"1,2,3,4,5" | split ","`);
            expect(retriever.retrieve(null)).toEqual(["1", "2", "3", "4", "5"]);
            retriever.delete();
            var retriever = parser.parse_expression(`"1,2,3,4,5" | split ","`);
            expect(retriever.retrieve(null)).toEqual(["1", "2", "3", "4", "5"]);
            retriever.delete();
        } finally {
            parser.delete();
        }
    });

    test("Accessor", async () => {
        var parser = await siplus();
        
        try {
            let retriever = parser.parse_expression(`.fake`);
            expect(retriever.retrieve({})).toEqual(undefined);
            retriever.delete();

            retriever = parser.parse_expression(`.b`);
            expect(retriever.retrieve({ b: 2 })).toEqual(2);
            retriever.delete();

            retriever = parser.parse_interpolation(`{ .fake }`);
            expect(retriever.construct({})).toEqual("");
            retriever.delete();
        } finally {
            parser.delete();
        }
    });
})
