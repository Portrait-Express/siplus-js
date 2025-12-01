const { siplus, getParametersFirstParent } = require("../dist/index.js");

describe ('SIPlus Tests', () => {
    test("Parser", async () => {
        var parser = await siplus();

        var retriever = parser.parse_interpolation("Hello {.id}");
        expect(retriever.construct({id: 1})).toEqual("Hello 1");
        retriever.delete();

        var retriever = parser.parse_expression("map . .id");
        expect(retriever.retrieve([{ id: 1 }, { id: 2 }]))
            .toEqual([1, 2]);

        retriever.delete();
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
})
