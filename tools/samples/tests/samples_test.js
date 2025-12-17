#!/usr/bin/env node
/**
 * JavaScript/Node.js test for WASM bindings of IDL samples.
 * 
 * Build WASM first:
 *   cd tools/samples/build-wasm
 *   source ~/emsdk/emsdk_env.sh
 *   emcmake cmake .. -GNinja
 *   ninja
 * 
 * Run test:
 *   node tools/samples/build-wasm/samples_test.js
 */

const path = require('path');

// Load the WASM module
const wasmPath = path.join(__dirname, 'samples_wasm.js');

let SamplesModule;
try {
    SamplesModule = require(wasmPath);
} catch (e) {
    console.error('Error: Could not load WASM module from:', wasmPath);
    console.error('Make sure to build the WASM module first.');
    console.error('Run: cd tools/samples && source ~/emsdk/emsdk_env.sh && mkdir -p build-wasm && cd build-wasm && emcmake cmake .. -GNinja && ninja');
    process.exit(1);
}

// Initialize module and run tests
SamplesModule().then(function(Module) {
    console.log('=== IDL Samples JavaScript/WASM Test ===\n');
    
    let allPassed = true;
    
    allPassed &= testCalculator(Module);
    allPassed &= testGeometry(Module);
    allPassed &= testShapeProcessor(Module);
    allPassed &= testAsyncProcessor(Module);
    
    console.log('\n=== Summary ===');
    if (allPassed) {
        console.log('All tests PASSED!');
        process.exit(0);
    } else {
        console.log('Some tests FAILED!');
        process.exit(1);
    }
}).catch(function(e) {
    console.error('Error initializing WASM module:', e);
    process.exit(1);
});

function assertEquals(name, expected, actual) {
    const passed = expected === actual;
    if (!passed) {
        console.log(`    FAIL: ${name} - expected ${expected}, got ${actual}`);
    }
    return passed;
}

function assertClose(name, expected, actual, tolerance = 0.0001) {
    const passed = Math.abs(expected - actual) < tolerance;
    if (!passed) {
        console.log(`    FAIL: ${name} - expected ${expected}, got ${actual}`);
    }
    return passed;
}

function testCalculator(Module) {
    console.log('Testing Calculator...');
    let passed = true;
    
    try {
        const calc = new Module.Calculator();
        if (!calc.create()) {
            console.log('  FAIL: Could not create Calculator');
            return false;
        }
        
        passed &= assertEquals('add(2, 3)', 5, calc.add(2, 3));
        passed &= assertEquals('subtract(10, 4)', 6, calc.subtract(10, 4));
        passed &= assertEquals('multiply(3, 7)', 21, calc.multiply(3, 7));
        passed &= assertClose('divide(10, 4)', 2.5, calc.divide(10.0, 4.0));
        passed &= assertEquals('getVersionMajor()', 1, calc.getVersionMajor());
        passed &= assertEquals('getVersionMinor()', 0, calc.getVersionMinor());
        
        calc.delete();
        
        console.log('  Calculator: ' + (passed ? 'PASSED' : 'FAILED'));
    } catch (e) {
        console.log('  Calculator: FAILED with exception:', e.message);
        return false;
    }
    
    return passed;
}

function testGeometry(Module) {
    console.log('Testing Geometry...');
    let passed = true;
    
    try {
        const geom = new Module.Geometry();
        if (!geom.create()) {
            console.log('  FAIL: Could not create Geometry');
            return false;
        }
        
        const line = geom.createLine(0, 0, 100, 100, 5);
        passed &= assertEquals('createLine length', 5, line.length);
        passed &= assertEquals('createLine[0].x', 0, line[0].x);
        passed &= assertEquals('createLine[4].x', 100, line[4].x);
        
        const boxes = geom.findBoundingBoxes(3);
        passed &= assertEquals('findBoundingBoxes length', 3, boxes.length);
        passed &= assertEquals('boxes[0].x', 0, boxes[0].x);
        passed &= assertEquals('boxes[1].x', 10, boxes[1].x);
        
        passed &= assertEquals('getLastCount()', 3, geom.getLastCount());
        
        geom.delete();
        
        console.log('  Geometry: ' + (passed ? 'PASSED' : 'FAILED'));
    } catch (e) {
        console.log('  Geometry: FAILED with exception:', e.message);
        return false;
    }
    
    return passed;
}

function testShapeProcessor(Module) {
    console.log('Testing ShapeProcessor...');
    let passed = true;
    
    try {
        const processor = new Module.ShapeProcessor();
        if (!processor.create()) {
            console.log('  FAIL: Could not create ShapeProcessor');
            return false;
        }
        
        const box = { x: 10, y: 20, width: 100, height: 50, confidence: 0.9 };
        passed &= assertEquals('calculateArea', 5000, processor.calculateArea(box));
        
        const box2 = { x: 0, y: 0, width: 3, height: 4, confidence: 0.9 };
        passed &= assertClose('calculateDiagonal', 5.0, processor.calculateDiagonal(box2));
        
        const p = { x: 10, y: 20 };
        const translated = processor.translate(p, 5, -3);
        passed &= assertEquals('translate.x', 15, translated.x);
        passed &= assertEquals('translate.y', 17, translated.y);
        
        const created = processor.createBox(1, 2, 3, 4);
        passed &= assertEquals('createBox.x', 1, created.x);
        passed &= assertEquals('createBox.y', 2, created.y);
        passed &= assertEquals('createBox.width', 3, created.width);
        passed &= assertEquals('createBox.height', 4, created.height);
        
        const testBox = { x: 0, y: 0, width: 100, height: 100, confidence: 1.0 };
        const inside = { x: 50, y: 50 };
        const outside = { x: 150, y: 150 };
        passed &= assertEquals('boxContainsPoint(inside)', true, processor.boxContainsPoint(testBox, inside));
        passed &= assertEquals('boxContainsPoint(outside)', false, processor.boxContainsPoint(testBox, outside));
        
        processor.delete();
        
        console.log('  ShapeProcessor: ' + (passed ? 'PASSED' : 'FAILED'));
    } catch (e) {
        console.log('  ShapeProcessor: FAILED with exception:', e.message);
        return false;
    }
    
    return passed;
}

function testAsyncProcessor(Module) {
    console.log('Testing AsyncProcessor...');
    let passed = true;
    
    try {
        const processor = new Module.AsyncProcessor();
        if (!processor.create()) {
            console.log('  FAIL: Could not create AsyncProcessor');
            return false;
        }
        
        // Test processWithProgress with callback
        const progressCalls = [];
        const result = processor.processWithProgress(5, function(current, total) {
            progressCalls.push({ current, total });
            console.log(`    Progress: ${current}/${total}`);
        });
        passed &= assertEquals('processWithProgress result', 5, result);
        passed &= assertEquals('progress calls count', 5, progressCalls.length);
        
        // Test countFiltered with filter callback
        const evenCount = processor.countFiltered(1, 10, function(value) {
            return value % 2 === 0;
        });
        passed &= assertEquals('countFiltered (evens)', 5, evenCount);
        
        // Test sumTransformed with transform callback
        const sumSquares = processor.sumTransformed(1, 5, function(value) {
            return value * value;
        });
        passed &= assertEquals('sumTransformed (squares)', 55, sumSquares);
        
        processor.delete();
        
        console.log('  AsyncProcessor: ' + (passed ? 'PASSED' : 'FAILED'));
    } catch (e) {
        console.log('  AsyncProcessor: FAILED with exception:', e.message);
        return false;
    }
    
    return passed;
}
