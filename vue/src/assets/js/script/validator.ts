// export interface ValidationError {
//     key: string;
//     line: number;
//     position: number;
//     token?: string;
//     params?: Record<string, string | number>;
// }

// export interface ValidationResult {
//     valid: boolean;
//     errors: ValidationError[];
// }

// export interface ValidationContext {
//     script: string;
//     pos: number;
//     line: number;
//     column: number;
//     errors: ValidationError[];
//     maxErrors: number;
//     ifDepth: number;
//     whileDepth: number;
//     onDepth: number;
//     hasElse: boolean;
//     hasContent: boolean;
//     parenDepth: number;
//     arrayDepth: number;
//     inError: boolean;
//     token: string;
//     lastWasTime: boolean;
//     lastWasFloat: boolean;
//     currentFunction: string;
// }

// // ============================================================
// // ================ 2. COMMAND INTERFACES (command.ts) =========
// // ============================================================

// export interface CommandConfig {
//     name: string;
//     pattern: string;
//     description?: string;
//     requiredArgs?: number;
//     maxDepth?: number;
//     allowNesting?: boolean;
//     argTypes?: string[];
//     customValidator?: (args: any[], context: ValidationContext) => { valid: boolean; error?: string; params?: Record<string, any> };
// }

// export interface CommandHandler {
//     name: string;
//     priority: number;
//     match(context: ValidationContext): boolean;
//     validate(engine: ValidatorEngine): void;
// }

// export interface VariableValidator {
//     name: string;
//     priority: number;
//     validate(context: ValidationContext, varName: string): boolean;
// }

// export interface PostValidationHook {
//     name: string;
//     priority: number;
//     execute(context: ValidationContext, result: ValidationResult): ValidationResult;
// }

// export interface ExpressionHandler {
//     name: string;
//     priority: number;
//     canHandle(engine: ValidatorEngine): boolean;
//     parse(engine: ValidatorEngine): any;
// }

// // ============================================================
// // ================ 3. ENGINE CORE (engine.ts) ================
// // ============================================================

// export class ValidatorEngine {
//     private context!: ValidationContext;
//     private commandHandlers: CommandHandler[] = [];
//     private variableValidators: VariableValidator[] = [];
//     private postHooks: PostValidationHook[] = [];
//     private expressionHandlers: ExpressionHandler[] = [];
//     private commandConfigs: Map<string, CommandConfig> = new Map();
    
//     private static readonly MAX_ERRORS = 32;
//     private static readonly MAX_SCRIPT = 256;
//     private static readonly MAX_STRING_LEN = 32;
//     private static readonly MAX_ARRAY_SIZE = 64;
//     private static readonly MAX_CALL_ID = 255;

//     private static readonly FUNCTIONS = new Set([
//         'len', 'get', 'set', 'chr', 'ord', 'call', 'wait', 'display'
//     ]);
    
//     private static readonly OPERATORS = new Set([
//         '+', '-', '*', '/', '%', '&', '|', '^', '~'
//     ]);

//     constructor() {
//         this.registerDefaults();
//     }

//     private registerDefaults(): void {
//         this.registerCommand(new EndCommandHandler());
//         this.registerCommand(new ElseCommandHandler());
//         this.registerCommand(new IfCommandHandler());
//         this.registerCommand(new WhileCommandHandler());
//         this.registerCommand(new OnCommandHandler());
//         this.registerCommand(new AssignmentCommandHandler());
//         this.registerCommand(new FunctionCallCommandHandler());

//         this.registerVariableValidator(new PortValidator());
//         this.registerVariableValidator(new ArrayVarValidator());
//         this.registerVariableValidator(new StringVarValidator());
//         this.registerVariableValidator(new NumberVarValidator());
//         this.registerVariableValidator(new FloatVarValidator());
//         this.registerVariableValidator(new IntVarValidator());

//         this.registerPostHook(new BlockBalanceHook());
//         this.registerExpressionHandler(new TimeExpressionHandler());
//     }

//     registerCommand(handler: CommandHandler): void {
//         this.commandHandlers.push(handler);
//         this.commandHandlers.sort((a, b) => b.priority - a.priority);
//     }

//     registerCustomCommand(config: CommandConfig): void {
//         this.commandConfigs.set(config.name, config);
//         const handler = new CustomCommandHandler(config);
//         this.registerCommand(handler);
//     }

//     registerVariableValidator(validator: VariableValidator): void {
//         this.variableValidators.push(validator);
//         this.variableValidators.sort((a, b) => b.priority - a.priority);
//     }

//     registerPostHook(hook: PostValidationHook): void {
//         this.postHooks.push(hook);
//         this.postHooks.sort((a, b) => b.priority - a.priority);
//     }

//     registerExpressionHandler(handler: ExpressionHandler): void {
//         this.expressionHandlers.push(handler);
//         this.expressionHandlers.sort((a, b) => b.priority - a.priority);
//     }

//     validate(script: string): ValidationResult {
//         this.initContext(script);
        
//         if (!this.context.script || this.context.script.trim().length === 0) {
//             this.addError('scriptEmpty');
//             return this.getResult();
//         }

//         if (this.context.script.length > ValidatorEngine.MAX_SCRIPT) {
//             this.addError('scriptTooLong', { max: ValidatorEngine.MAX_SCRIPT });
//             return this.getResult();
//         }

//         this.parse();

//         let result = this.getResult();
//         for (const hook of this.postHooks) {
//             result = hook.execute(this.context, result);
//         }

//         return result;
//     }

//     private initContext(script: string): void {
//         this.context = {
//             script,
//             pos: 0,
//             line: 1,
//             column: 1,
//             errors: [],
//             maxErrors: ValidatorEngine.MAX_ERRORS,
//             ifDepth: 0,
//             whileDepth: 0,
//             onDepth: 0,
//             hasElse: false,
//             hasContent: false,
//             parenDepth: 0,
//             arrayDepth: 0,
//             inError: false,
//             token: '',
//             lastWasTime: false,
//             lastWasFloat: false,
//             currentFunction: ''
//         };
//     }

//     private parse(): void {
//         while (this.peek() !== '\0' && this.context.errors.length < this.context.maxErrors) {
//             this.skipWhitespace();
//             if (this.peek() === '\0') break;
//             this.context.inError = false;
//             this.context.hasContent = true;
            
//             let handled = false;
//             for (const handler of this.commandHandlers) {
//                 if (handler.match(this.context)) {
//                     handler.validate(this);
//                     handled = true;
//                     break;
//                 }
//             }
            
//             if (!handled) {
//                 const cmd = this.readToken();
//                 this.addError('unknownCommand', { command: cmd }, cmd);
//                 this.context.inError = true;
//             }
            
//             if (this.context.inError) {
//                 this.skipToNextCommand();
//             }
//         }
//     }

//     parseBlock(): void {
//         while (this.peek() !== '\0' && this.context.errors.length < this.context.maxErrors) {
//             this.skipWhitespace();
//             if (this.peek() === '\0') break;
            
//             if (this.context.ifDepth > 0 || this.context.whileDepth > 0 || this.context.onDepth > 0) {
//                 const savePos = this.context.pos;
//                 const saveLine = this.context.line;
//                 const saveColumn = this.context.column;
//                 if (this.matchString('end')) {
//                     this.context.pos = savePos;
//                     this.context.line = saveLine;
//                     this.context.column = saveColumn;
//                     break;
//                 }
//             }
            
//             this.context.inError = false;
//             this.context.hasContent = true;
            
//             let handled = false;
//             for (const handler of this.commandHandlers) {
//                 if (handler.match(this.context)) {
//                     handler.validate(this);
//                     handled = true;
//                     break;
//                 }
//             }
            
//             if (!handled) {
//                 const cmd = this.readToken();
//                 this.addError('unknownCommand', { command: cmd }, cmd);
//                 this.context.inError = true;
//             }
            
//             if (this.context.inError) {
//                 this.skipToNextCommand();
//             }
//         }
        
//         if (!this.context.hasContent && (this.context.ifDepth > 0 || this.context.whileDepth > 0 || this.context.onDepth > 0)) {
//             const block = this.context.ifDepth > 0 ? 'if' : this.context.whileDepth > 0 ? 'while' : 'on';
//             this.addError('emptyBlock', { block });
//         }
//     }

//     // ============================================================
//     // ================ 4. CONTEXT HELPERS ========================
//     // ============================================================

//     getScript(): string { return this.context.script; }
//     getPos(): number { return this.context.pos; }
//     getLine(): number { return this.context.line; }
//     getColumn(): number { return this.context.column; }
//     getErrors(): ValidationError[] { return this.context.errors; }
    
//     get ifDepth(): number { return this.context.ifDepth; }
//     set ifDepth(value: number) { this.context.ifDepth = value; }
//     get whileDepth(): number { return this.context.whileDepth; }
//     set whileDepth(value: number) { this.context.whileDepth = value; }
//     get onDepth(): number { return this.context.onDepth; }
//     set onDepth(value: number) { this.context.onDepth = value; }
//     get hasElse(): boolean { return this.context.hasElse; }
//     set hasElse(value: boolean) { this.context.hasElse = value; }
//     get hasContent(): boolean { return this.context.hasContent; }
//     set hasContent(value: boolean) { this.context.hasContent = value; }
//     get inError(): boolean { return this.context.inError; }
//     set inError(value: boolean) { this.context.inError = value; }
//     get token(): string { return this.context.token; }
//     set token(value: string) { this.context.token = value; }
//     get lastWasTime(): boolean { return this.context.lastWasTime; }
//     set lastWasTime(value: boolean) { this.context.lastWasTime = value; }
//     get lastWasFloat(): boolean { return this.context.lastWasFloat; }
//     set lastWasFloat(value: boolean) { this.context.lastWasFloat = value; }
//     get currentFunction(): string { return this.context.currentFunction; }
//     set currentFunction(value: string) { this.context.currentFunction = value; }

//     // ============================================================
//     // ================ 5. PARSING HELPERS ========================
//     // ============================================================

//     peek(): string {
//         if (this.context.pos >= this.context.script.length) return '\0';
//         return this.context.script[this.context.pos];
//     }

//     peekNext(): string {
//         if (this.context.pos + 1 >= this.context.script.length) return '\0';
//         return this.context.script[this.context.pos + 1];
//     }

//     next(): string {
//         const c = this.peek();
//         if (c !== '\0') {
//             this.context.pos++;
//             this.context.column++;
//             if (c === '\n') {
//                 this.context.line++;
//                 this.context.column = 1;
//             }
//         }
//         return c;
//     }

//     skipWhitespace(): void {
//         while (this.peek() !== '\0') {
//             const c = this.peek();
//             if (c === ' ' || c === '\t' || c === '\r') {
//                 this.next();
//             } else if (c === '\n') {
//                 this.context.line++;
//                 this.context.column = 1;
//                 this.next();
//             } else {
//                 break;
//             }
//         }
//     }

//     skipToNextCommand(): void {
//         while (this.peek() !== '\0') {
//             const c = this.peek();
//             if (c === ';') { this.next(); return; }
//             if (c === '\n') { this.next(); return; }
//             this.next();
//         }
//     }

//     matchString(str: string): boolean {
//         if (this.context.pos + str.length > this.context.script.length) return false;
//         const savePos = this.context.pos, saveLine = this.context.line, saveColumn = this.context.column;
//         for (let i = 0; i < str.length; i++) {
//             if (this.peek() !== str[i]) {
//                 this.context.pos = savePos;
//                 this.context.line = saveLine;
//                 this.context.column = saveColumn;
//                 return false;
//             }
//             this.next();
//         }
//         return true;
//     }

//     peekMatch(str: string): boolean {
//         if (this.context.pos + str.length > this.context.script.length) return false;
//         for (let i = 0; i < str.length; i++) {
//             if (this.context.script[this.context.pos + i] !== str[i]) {
//                 return false;
//             }
//         }
//         return true;
//     }

//     readToken(): string {
//         this.skipWhitespace();
//         let token = '';
//         while (this.peek() !== '\0') {
//             const c = this.peek();
//             if (c === ' ' || c === '\t' || c === '\n' || c === '\r') break;
//             if (';()=,{}\''.includes(c)) break;
//             token += this.next();
//         }
//         this.context.token = token;
//         return token;
//     }

//     isDigit(c: string): boolean { return c >= '0' && c <= '9'; }
//     isLetter(c: string): boolean { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
//     isAlphaNumeric(c: string): boolean { return this.isDigit(c) || this.isLetter(c); }
//     isVariableChar(c: string): boolean { return this.isAlphaNumeric(c) || c === '_'; }
//     isWhitespace(c: string): boolean { return c === ' ' || c === '\t' || c === '\n' || c === '\r'; }
    
//     isOperator(c: string): boolean { 
//         return ValidatorEngine.OPERATORS.has(c); 
//     }
    
//     isCompoundOperator(): boolean {
//         const c = this.peek();
//         const next = this.peekNext();
//         return (c === '<' && next === '<') ||
//                (c === '>' && next === '>') ||
//                (c === '&' && next === '&') ||
//                (c === '|' && next === '|');
//     }

//     parseInt(): number {
//         const result = this.parseNumberOrTime();
//         return Math.floor(result.value);
//     }

//     parseFloat(): number {
//         const result = this.parseNumberOrTime();
//         return result.value;
//     }

//     parseNumberOrTime(): { value: number; isTime: boolean; isFloat?: boolean } {
//         let value = 0;
//         let floatValue = 0;
//         let decimalPlaces = 0;
//         let negative = false;
//         let isTime = false;
//         let isFloat = false;
        
//         this.skipWhitespace();
//         if (this.peek() === '-') { negative = true; this.next(); }
        
//         while (this.isDigit(this.peek())) {
//             value = value * 10 + parseInt(this.next());
//         }
        
//         if (this.peek() === '.') {
//             isFloat = true;
//             this.next();
//             while (this.isDigit(this.peek())) {
//                 floatValue = floatValue * 10 + parseInt(this.next());
//                 decimalPlaces++;
//             }
//         }
        
//         let numberValue = value;
//         if (isFloat) {
//             numberValue = value + (floatValue / Math.pow(10, decimalPlaces));
//             if (negative) numberValue = -numberValue;
//         } else {
//             numberValue = negative ? -value : value;
//         }
        
//         if (!isFloat) {
//             let suffix = '';
//             while (this.isLetter(this.peek())) {
//                 suffix += this.next();
//             }
            
//             if (suffix === 's' || suffix === 'ms' || suffix === 'us') {
//                 isTime = true;
//                 if (negative) {
//                     this.addError('timeNegative');
//                     return { value: 0, isTime: false };
//                 }
//                 if (value === 0) {
//                     this.addError('timeZero');
//                     return { value: 0, isTime: false };
//                 }
//             } else if (suffix.length > 0) {
//                 this.addError('invalidSuffix', { suffix });
//                 return { value: 0, isTime: false };
//             }
//         }
        
//         return { value: numberValue, isTime, isFloat };
//     }

//     parseExpression(): boolean {
//         this.skipWhitespace();
//         if (!this.parseOperand()) return false;
//         this.skipWhitespace();
        
//         while (true) {
//             let op = '';
            
//             if (this.isCompoundOperator()) {
//                 if (this.peek() === '<' && this.peekNext() === '<') {
//                     op = '<<';
//                     this.next(); this.next();
//                 } else if (this.peek() === '>' && this.peekNext() === '>') {
//                     op = '>>';
//                     this.next(); this.next();
//                 } else if (this.peek() === '&' && this.peekNext() === '&') {
//                     op = '&&';
//                     this.next(); this.next();
//                 } else if (this.peek() === '|' && this.peekNext() === '|') {
//                     op = '||';
//                     this.next(); this.next();
//                 }
//             } else if (this.isOperator(this.peek())) {
//                 op = this.next();
//             } else {
//                 break;
//             }
            
//             this.skipWhitespace();
            
//             if (op === '/') {
//                 this.skipWhitespace();
//                 const result = this.parseNumberOrTime();
//                 if (result.value === 0) {
//                     this.addError('divisionByZero');
//                     return false;
//                 }
//                 if (result.isTime) {
//                     this.addError('timeInArithmetic');
//                     return false;
//                 }
//             }
            
//             if (!this.parseOperand()) return false;
//             this.skipWhitespace();
//         }
//         return true;
//     }

//     parseCondition(): boolean {
//         if (!this.parseExpression()) return false;
//         this.skipWhitespace();
        
//         let hasComparison = false;
        
//         if (this.peek() === '=' && this.peekNext() === '=') {
//             this.next(); this.next();
//             hasComparison = true;
//         } else if (this.peek() === '!' && this.peekNext() === '=') {
//             this.next(); this.next();
//             hasComparison = true;
//         } else if (this.peek() === '>' && this.peekNext() === '=') {
//             this.next(); this.next();
//             hasComparison = true;
//         } else if (this.peek() === '<' && this.peekNext() === '=') {
//             this.next(); this.next();
//             hasComparison = true;
//         } else if (this.peek() === '>') {
//             this.next();
//             hasComparison = true;
//         } else if (this.peek() === '<') {
//             this.next();
//             hasComparison = true;
//         }
        
//         if (!hasComparison) {
//             this.addError('expectedComparison');
//             return false;
//         }
        
//         this.skipWhitespace();
        
//         if (!this.parseExpression()) return false;
//         this.skipWhitespace();
        
//         while (true) {
//             if (this.peek() === '&' && this.peekNext() === '&') {
//                 this.next(); this.next();
//             } else if (this.peek() === '|' && this.peekNext() === '|') {
//                 this.next(); this.next();
//             } else {
//                 break;
//             }
            
//             this.skipWhitespace();
            
//             if (!this.parseExpression()) return false;
//             this.skipWhitespace();
            
//             let hasComp = false;
            
//             if (this.peek() === '=' && this.peekNext() === '=') {
//                 this.next(); this.next();
//                 hasComp = true;
//             } else if (this.peek() === '!' && this.peekNext() === '=') {
//                 this.next(); this.next();
//                 hasComp = true;
//             } else if (this.peek() === '>' && this.peekNext() === '=') {
//                 this.next(); this.next();
//                 hasComp = true;
//             } else if (this.peek() === '<' && this.peekNext() === '=') {
//                 this.next(); this.next();
//                 hasComp = true;
//             } else if (this.peek() === '>') {
//                 this.next();
//                 hasComp = true;
//             } else if (this.peek() === '<') {
//                 this.next();
//                 hasComp = true;
//             }
            
//             if (!hasComp) {
//                 this.addError('expectedComparison');
//                 return false;
//             }
            
//             this.skipWhitespace();
//             if (!this.parseExpression()) return false;
//             this.skipWhitespace();
//         }
        
//         return true;
//     }

//     parseOperand(): boolean {
//         this.skipWhitespace();
//         const c = this.peek();
//         if (c === '\0') {
//             this.addError('expectedExpression');
//             return false;
//         }
        
//         if (this.isDigit(c) || c === '-') {
//             const result = this.parseNumberOrTime();
//             this.context.lastWasTime = result.isTime;
//             this.context.lastWasFloat = result.isFloat || false;
//             return true;
//         }
        
//         if (c === '$') {
//             const varName = this.readToken();
//             if (!this.checkVariable(varName)) return false;
//             this.context.lastWasTime = false;
//             this.context.lastWasFloat = false;
//             return true;
//         }
        
//         if (c === "'") {
//             this.context.lastWasTime = false;
//             this.context.lastWasFloat = false;
//             return this.parseString();
//         }
        
//         if (c === '{') {
//             this.context.lastWasTime = false;
//             this.context.lastWasFloat = false;
//             return this.parseArray();
//         }
        
//         if (this.isLetter(c)) {
//             this.context.lastWasTime = false;
//             this.context.lastWasFloat = false;
//             for (const handler of this.expressionHandlers) {
//                 if (handler.canHandle(this)) {
//                     handler.parse(this);
//                     return true;
//                 }
//             }
//             return this.parseFunctionCall();
//         }
        
//         if (c === '(') {
//             this.next();
//             if (!this.parseExpression()) return false;
//             this.skipWhitespace();
//             if (this.peek() !== ')') {
//                 this.addError('unclosedParenthesis');
//                 return false;
//             }
//             this.next();
//             this.context.lastWasTime = false;
//             this.context.lastWasFloat = false;
//             return true;
//         }
        
//         if (c === '=' || c === '!' || c === '>' || c === '<') {
//             return true;
//         }
        
//         this.addError('expectedValue');
//         return false;
//     }

//     parseString(): boolean {
//         if (this.peek() !== "'") {
//             this.addError('expectedString');
//             return false;
//         }
//         this.next();
//         let strLen = 0;
//         while (this.peek() !== '\0') {
//             if (this.peek() === "'") {
//                 this.next();
//                 if (strLen > ValidatorEngine.MAX_STRING_LEN) {
//                     this.addError('stringTooLong', {
//                         length: strLen,
//                         max: ValidatorEngine.MAX_STRING_LEN
//                     });
//                 }
//                 return true;
//             }
//             if (this.peek() === '\n') {
//                 this.addError('unclosedString');
//                 return false;
//             }
//             strLen++;
//             this.next();
//         }
//         this.addError('unclosedString');
//         return false;
//     }

//     parseArray(): boolean {
//         if (this.peek() !== '{') {
//             this.addError('expectedArray');
//             return false;
//         }
//         this.next();
//         this.context.arrayDepth++;
//         let elements = 0;
//         while (this.peek() !== '\0') {
//             this.skipWhitespace();
//             if (this.peek() === '}') {
//                 this.next();
//                 this.context.arrayDepth--;
//                 if (elements === 0) {
//                     this.addError('emptyBlock', { block: 'array' });
//                     return false;
//                 }
//                 return true;
//             }
//             if (!this.parseExpression()) return false;
//             elements++;
//             if (elements > ValidatorEngine.MAX_ARRAY_SIZE) {
//                 this.addError('arrayTooLarge', {
//                     size: elements,
//                     max: ValidatorEngine.MAX_ARRAY_SIZE
//                 });
//                 return false;
//             }
//             this.skipWhitespace();
//             if (this.peek() === ',') { this.next(); continue; }
//             else if (this.peek() === '}') continue;
//             else {
//                 this.addError('syntax');
//                 return false;
//             }
//         }
//         this.addError('unclosedArray');
//         return false;
//     }

//     parseFunctionCall(): boolean {
//         const funcName = this.readToken();
//         const isKnown = ValidatorEngine.FUNCTIONS.has(funcName);
        
//         if (!isKnown) {
//             this.addError('unknownCommand', { command: funcName }, funcName);
//             this.context.inError = true;
//         }
        
//         this.context.currentFunction = funcName;
//         this.skipWhitespace();
        
//         if (this.peek() !== '(') {
//             this.addError('unclosedParenthesis');
//             this.context.inError = true;
//             return false;
//         }
//         this.next();
//         this.skipWhitespace();

//         if (!isKnown) {
//             let parenCount = 1;
//             while (this.peek() !== '\0' && parenCount > 0 && this.context.errors.length < this.context.maxErrors) {
//                 const c = this.peek();
//                 if (c === '(') { parenCount++; this.next(); continue; }
//                 if (c === ')') { parenCount--; if (parenCount === 0) { this.next(); break; } this.next(); continue; }
//                 if (c === '\n') { this.addError('unclosedParenthesis'); this.context.inError = true; return false; }
//                 if (c === ';') { this.addError('unclosedParenthesis'); this.context.inError = true; return false; }
//                 if (c === '$') {
//                     const varName = this.readToken();
//                     this.checkVariable(varName);
//                     continue;
//                 }
//                 this.next();
//             }
//             if (parenCount > 0) {
//                 this.addError('unclosedParenthesis');
//                 this.context.inError = true;
//             }
//             this.skipWhitespace();
//             const next = this.peek();
//             if (next !== ';' && next !== '\0' && next !== '\n') {
//                 this.addError('missingSeparator');
//                 this.context.inError = true;
//             }
//             if (next === ';') this.next();
//             this.context.currentFunction = '';
//             return true;
//         }

//         const result = this.handleKnownFunction(funcName);
//         this.context.currentFunction = '';
//         return result;
//     }

//     private handleKnownFunction(funcName: string): boolean {
//         if (funcName === 'wait') {
//             if (this.peek() === ')') {
//                 this.addError('timeRequired');
//                 this.context.inError = true;
//                 return false;
//             }
//             const savePos = this.context.pos;
//             const saveLine = this.context.line;
//             const saveColumn = this.context.column;
//             if (!this.parseExpression()) {
//                 this.context.inError = true;
//                 return false;
//             }
//             if (!this.context.lastWasTime) {
//                 this.context.pos = savePos;
//                 this.context.line = saveLine;
//                 this.context.column = saveColumn;
//                 const result = this.parseNumberOrTime();
//                 if (!result.isTime) {
//                     this.addError('timeRequired');
//                     this.context.inError = true;
//                     return false;
//                 }
//                 if (result.value <= 0) {
//                     this.addError('timeZero');
//                     this.context.inError = true;
//                     return false;
//                 }
//                 if (result.isFloat) {
//                     this.addError('timeInvalidSuffix', { suffix: 'float' });
//                     this.context.inError = true;
//                     return false;
//                 }
//             }
//         }
//         else if (funcName === 'call') {
//             if (this.peek() === ')') {
//                 this.addError('invalidCallId', { id: 0, max: ValidatorEngine.MAX_CALL_ID });
//                 this.context.inError = true;
//                 return false;
//             }
//             const savePos = this.context.pos;
//             const saveLine = this.context.line;
//             const saveColumn = this.context.column;
//             if (!this.parseExpression()) {
//                 this.context.inError = true;
//                 return false;
//             }
//             this.context.pos = savePos;
//             this.context.line = saveLine;
//             this.context.column = saveColumn;
//             const callId = this.parseInt();
//             if (callId < 0 || callId > ValidatorEngine.MAX_CALL_ID) {
//                 this.addError('invalidCallId', { id: callId, max: ValidatorEngine.MAX_CALL_ID });
//                 this.context.inError = true;
//                 return false;
//             }
//         }
//         else if (funcName === 'chr') {
//             if (this.peek() === ')') {
//                 this.addError('chrValueOutOfRange', { value: 0 });
//                 this.context.inError = true;
//                 return false;
//             }
//             const savePos = this.context.pos;
//             const saveLine = this.context.line;
//             const saveColumn = this.context.column;
//             if (!this.parseExpression()) {
//                 this.context.inError = true;
//                 return false;
//             }
//             this.context.pos = savePos;
//             this.context.line = saveLine;
//             this.context.column = saveColumn;
//             const value = this.parseInt();
//             if (value < 0 || value > 255) {
//                 this.addError('chrValueOutOfRange', { value });
//                 this.context.inError = true;
//                 return false;
//             }
//         }
//         else if (funcName === 'get' || funcName === 'set') {
//             if (this.peek() === ')') {
//                 this.addError('expectedValue');
//                 this.context.inError = true;
//                 return false;
//             }
//             if (!this.parseExpression()) {
//                 this.context.inError = true;
//                 return false;
//             }
//             this.skipWhitespace();
//             if (this.peek() !== ',') {
//                 this.addError('syntax');
//                 this.context.inError = true;
//                 return false;
//             }
//             this.next();
//             this.skipWhitespace();
            
//             if (this.peek() === ')') {
//                 this.addError('expectedValue');
//                 this.context.inError = true;
//                 return false;
//             }
//             const savePos = this.context.pos;
//             const saveLine = this.context.line;
//             const saveColumn = this.context.column;
//             if (!this.parseExpression()) {
//                 this.context.inError = true;
//                 return false;
//             }
//             this.context.pos = savePos;
//             this.context.line = saveLine;
//             this.context.column = saveColumn;
//             const index = this.parseInt();
//             if (index < 0 || index >= ValidatorEngine.MAX_ARRAY_SIZE) {
//                 this.addError('arrayIndexOutOfBounds', { index, max: ValidatorEngine.MAX_ARRAY_SIZE - 1 });
//                 this.context.inError = true;
//                 return false;
//             }
            
//             if (funcName === 'set') {
//                 this.skipWhitespace();
//                 if (this.peek() !== ',') {
//                     this.addError('syntax');
//                     this.context.inError = true;
//                     return false;
//                 }
//                 this.next();
//                 this.skipWhitespace();
//                 if (this.peek() === ')') {
//                     this.addError('expectedValue');
//                     this.context.inError = true;
//                     return false;
//                 }
//                 if (!this.parseExpression()) {
//                     this.context.inError = true;
//                     return false;
//                 }
//             }
//         }
//         else if (funcName === 'ord') {
//             if (this.peek() === ')') {
//                 this.addError('expectedValue');
//                 this.context.inError = true;
//                 return false;
//             }
//             if (!this.parseExpression()) {
//                 this.context.inError = true;
//                 return false;
//             }
//             this.skipWhitespace();
//             if (this.peek() === ',') {
//                 this.next();
//                 this.skipWhitespace();
//                 if (this.peek() === ')') {
//                     this.addError('expectedValue');
//                     this.context.inError = true;
//                     return false;
//                 }
//                 const savePos = this.context.pos;
//                 const saveLine = this.context.line;
//                 const saveColumn = this.context.column;
//                 if (!this.parseExpression()) {
//                     this.context.inError = true;
//                     return false;
//                 }
//                 this.context.pos = savePos;
//                 this.context.line = saveLine;
//                 this.context.column = saveColumn;
//                 const index = this.parseInt();
//                 if (index < 0 || index >= ValidatorEngine.MAX_STRING_LEN) {
//                     this.addError('stringIndexOutOfBounds', { index, max: ValidatorEngine.MAX_STRING_LEN - 1 });
//                     this.context.inError = true;
//                     return false;
//                 }
//             }
//         }
//         else {
//             if (this.peek() !== ')') {
//                 if (!this.parseExpression()) {
//                     this.context.inError = true;
//                     return false;
//                 }
//                 while (this.peek() === ',') {
//                     this.next();
//                     this.skipWhitespace();
//                     if (!this.parseExpression()) {
//                         this.context.inError = true;
//                         return false;
//                     }
//                 }
//             }
//         }

//         this.skipWhitespace();
//         if (this.peek() !== ')') {
//             this.addError('unclosedParenthesis');
//             this.context.inError = true;
//             return false;
//         }
//         this.next();
//         this.skipWhitespace();
//         const next = this.peek();
//         if (next !== ';' && next !== '\0') {
//             this.addError('missingSeparator');
//             this.context.inError = true;
//         }
//         if (next === ';') this.next();
//         return true;
//     }

//     // ============================================================
//     // ================ 6. VALIDATORS =============================
//     // ============================================================

//     private checkVariable(name: string): boolean {
//         if (!name || name[0] !== '$') {
//             this.addError('invalidVariableName', { name });
//             return false;
//         }
        
//         let valid = true;
//         for (const validator of this.variableValidators) {
//             if (!validator.validate(this.context, name)) {
//                 valid = false;
//             }
//         }
//         return valid;
//     }

//     // ============================================================
//     // ================ 7. ERROR HANDLING =========================
//     // ============================================================

//     addError(key: string, params?: Record<string, string | number>, token?: string): void {
//         if (this.context.errors.length >= this.context.maxErrors) return;
//         this.context.errors.push({
//             key,
//             line: this.context.line,
//             position: this.context.pos,
//             token: token || this.context.token || undefined,
//             params
//         });
//     }

//     private getResult(): ValidationResult {
//         return {
//             valid: this.context.errors.length === 0,
//             errors: this.context.errors
//         };
//     }
// }

// // ============================================================
// // ================ 8. COMMAND HANDLERS =======================
// // ============================================================

// export class EndCommandHandler implements CommandHandler {
//     name = 'end';
//     priority = 100;

//     match(context: ValidationContext): boolean {
//         return context.script.startsWith('end', context.pos);
//     }

//     validate(engine: ValidatorEngine): void {
//         if (!engine.matchString('end')) return;

//         if (engine.ifDepth <= 0 && engine.whileDepth <= 0 && engine.onDepth <= 0) {
//             engine.addError('unexpectedEnd');
//             engine.inError = true;
//             return;
//         }
//         if (engine.hasElse && !engine.hasContent) {
//             engine.addError('emptyElse');
//             engine.inError = true;
//             return;
//         }
//         if (engine.ifDepth > 0) engine.ifDepth--;
//         else if (engine.whileDepth > 0) engine.whileDepth--;
//         else if (engine.onDepth > 0) engine.onDepth--;
//         engine.hasElse = false;
//         engine.hasContent = false;
        
//         engine.skipWhitespace();
//         const next = engine.peek();
//         if (next !== ';' && next !== '\0' && next !== '\n') {
//             engine.addError('missingSeparator');
//             engine.inError = true;
//         }
//         if (next === ';') engine.next();
//     }
// }

// export class ElseCommandHandler implements CommandHandler {
//     name = 'else';
//     priority = 90;

//     match(context: ValidationContext): boolean {
//         return context.script.startsWith('else', context.pos);
//     }

//     validate(engine: ValidatorEngine): void {
//         if (!engine.matchString('else')) return;

//         if (engine.ifDepth <= 0) {
//             engine.addError('elseWithoutIf');
//             engine.inError = true;
//             return;
//         }
//         if (engine.hasElse) {
//             engine.addError('multipleElse');
//             engine.inError = true;
//             return;
//         }
//         if (!engine.hasContent) {
//             engine.addError('emptyBlock', { block: 'if' });
//             engine.inError = true;
//             return;
//         }
//         engine.hasElse = true;
//         engine.hasContent = false;
        
//         engine.skipWhitespace();
//         const next = engine.peek();
//         if (next !== ';' && next !== '\0' && next !== '\n') {
//             engine.addError('missingSeparator');
//             engine.inError = true;
//         }
//         if (next === ';') engine.next();
//     }
// }

// export class IfCommandHandler implements CommandHandler {
//     name = 'if';
//     priority = 80;

//     match(context: ValidationContext): boolean {
//         return context.script.startsWith('if:', context.pos);
//     }

//     validate(engine: ValidatorEngine): void {
//         if (!engine.matchString('if:')) return;

//         if (engine.ifDepth >= 8) {
//             engine.addError('nestedBlock', { block: 'if', max: 8 });
//             engine.inError = true;
//             return;
//         }
//         if (!engine.parseCondition()) {
//             engine.inError = true;
//             return;
//         }
//         engine.skipWhitespace();
//         if (engine.peek() !== ';') {
//             engine.addError('missingSeparator');
//             engine.inError = true;
//             return;
//         }
//         engine.next();
//         engine.ifDepth++;
//         engine.hasElse = false;
//         engine.hasContent = false;
//         engine.parseBlock();
//     }
// }

// export class WhileCommandHandler implements CommandHandler {
//     name = 'while';
//     priority = 70;

//     match(context: ValidationContext): boolean {
//         return context.script.startsWith('while:', context.pos);
//     }

//     validate(engine: ValidatorEngine): void {
//         if (!engine.matchString('while:')) return;

//         if (engine.whileDepth >= 1) {
//             engine.addError('nestedLoop');
//             engine.inError = true;
//             return;
//         }
//         if (!engine.parseCondition()) {
//             engine.inError = true;
//             return;
//         }
//         engine.skipWhitespace();
//         if (engine.peek() !== ';') {
//             engine.addError('missingSeparator');
//             engine.inError = true;
//             return;
//         }
//         engine.next();
//         engine.whileDepth++;
//         engine.hasContent = false;
//         engine.parseBlock();
//     }
// }

// export class OnCommandHandler implements CommandHandler {
//     name = 'on';
//     priority = 60;

//     match(context: ValidationContext): boolean {
//         return context.script.startsWith('on(', context.pos);
//     }

//     validate(engine: ValidatorEngine): void {
//         if (!engine.matchString('on(')) return;

//         if (engine.onDepth >= 8) {
//             engine.addError('nestedBlock', { block: 'on', max: 8 });
//             engine.inError = true;
//             return;
//         }
//         if (!engine.parseString()) {
//             engine.inError = true;
//             return;
//         }
//         engine.skipWhitespace();
//         if (engine.peek() !== ')') {
//             engine.addError('unclosedParenthesis');
//             engine.inError = true;
//             return;
//         }
//         engine.next();
//         engine.skipWhitespace();
//         if (engine.peek() !== ';') {
//             engine.addError('missingSeparator');
//             engine.inError = true;
//             return;
//         }
//         engine.next();
//         engine.onDepth++;
//         engine.hasContent = false;
//         engine.parseBlock();
//     }
// }

// export class AssignmentCommandHandler implements CommandHandler {
//     name = 'assignment';
//     priority = 50;

//     match(context: ValidationContext): boolean {
//         return context.script[context.pos] === '$';
//     }

//     validate(engine: ValidatorEngine): void {
//         const varName = engine.readToken();
//         if (varName[0] !== '$') {
//             engine.addError('expectedVariable');
//             engine.inError = true;
//             return;
//         }
        
//         if (!engine['checkVariable'](varName)) {
//             engine.inError = true;
//         }
        
//         engine.skipWhitespace();
//         if (engine.peek() !== '=') {
//             engine.addError('expectedAssignment');
//             engine.inError = true;
//             return;
//         }
        
//         engine.next();
//         engine.skipWhitespace();
//         if (!engine.parseExpression()) {
//             engine.inError = true;
//             return;
//         }
        
//         engine.skipWhitespace();
//         const next = engine.peek();
//         if (next !== ';' && next !== '\0') {
//             engine.addError('missingSeparator');
//             engine.inError = true;
//         }
//         if (next === ';') engine.next();
//     }
// }

// export class FunctionCallCommandHandler implements CommandHandler {
//     name = 'function-call';
//     priority = 40;

//     match(context: ValidationContext): boolean {
//         const c = context.script[context.pos];
//         return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
//     }

//     validate(engine: ValidatorEngine): void {
//         engine.parseFunctionCall();
//     }
// }

// export class CustomCommandHandler implements CommandHandler {
//     name: string;
//     priority = 30;
//     private config: CommandConfig;

//     constructor(config: CommandConfig) {
//         this.name = config.name;
//         this.config = config;
//     }

//     match(context: ValidationContext): boolean {
//         return context.script.startsWith(this.config.pattern, context.pos);
//     }

//     validate(engine: ValidatorEngine): void {
//         engine.matchString(this.config.pattern);
        
//         if (this.config.maxDepth !== undefined) {
//             const depth = this.getDepth(engine);
//             if (depth > this.config.maxDepth) {
//                 engine.addError('nestedBlock', {
//                     block: this.config.name,
//                     max: this.config.maxDepth
//                 });
//                 engine.inError = true;
//                 return;
//             }
//         }

//         engine.skipWhitespace();
//         const args: any[] = [];
        
//         if (this.config.requiredArgs && this.config.requiredArgs > 0) {
//             if (engine.peek() !== '(') {
//                 engine.addError('unclosedParenthesis');
//                 engine.inError = true;
//                 return;
//             }
//             engine.next();
//             engine.skipWhitespace();
            
//             while (engine.peek() !== ')' && engine.peek() !== '\0') {
//                 const result = engine.parseNumberOrTime();
//                 if (result.isTime) {
//                     args.push(result.value);
//                 } else {
//                     if (!engine.parseExpression()) {
//                         engine.inError = true;
//                         return;
//                     }
//                 }
//                 engine.skipWhitespace();
//                 if (engine.peek() === ',') {
//                     engine.next();
//                     engine.skipWhitespace();
//                 }
//             }
            
//             if (engine.peek() !== ')') {
//                 engine.addError('unclosedParenthesis');
//                 engine.inError = true;
//                 return;
//             }
//             engine.next();
//         }

//         if (this.config.customValidator) {
//             const result = this.config.customValidator(args, engine['context']);
//             if (!result.valid) {
//                 engine.addError(result.error || 'syntax', result.params);
//                 engine.inError = true;
//                 return;
//             }
//         }

//         engine.skipWhitespace();
//         const next = engine.peek();
//         if (next !== ';' && next !== '\0' && next !== '\n') {
//             engine.addError('missingSeparator');
//             engine.inError = true;
//         }
//         if (next === ';') engine.next();
//     }

//     private getDepth(engine: ValidatorEngine): number {
//         const name = this.config.name;
//         if (name === 'if') return engine.ifDepth;
//         if (name === 'while') return engine.whileDepth;
//         if (name === 'on') return engine.onDepth;
//         return 0;
//     }
// }

// // ============================================================
// // ================ 9. VARIABLE VALIDATORS ====================
// // ============================================================

// export class PortValidator implements VariableValidator {
//     name = 'port';
//     priority = 100;
//     maxPort = 39;

//     validate(context: ValidationContext, varName: string): boolean {
//         if (!varName.startsWith('$p')) return true;
//         const index = parseInt(varName.substring(2));
//         if (isNaN(index) || index > this.maxPort || index < 0) {
//             context.errors.push({
//                 key: 'invalidPort',
//                 line: context.line,
//                 position: context.pos,
//                 token: varName,
//                 params: { port: index, max: this.maxPort }
//             });
//             return false;
//         }
//         return true;
//     }
// }

// export class ArrayVarValidator implements VariableValidator {
//     name = 'array-var';
//     priority = 90;
//     maxIndex = 4;

//     validate(context: ValidationContext, varName: string): boolean {
//         if (!varName.startsWith('$a')) return true;
//         const index = parseInt(varName.substring(2));
//         if (isNaN(index) || index > this.maxIndex || index < 0) {
//             context.errors.push({
//                 key: 'invalidVariableIndex',
//                 line: context.line,
//                 position: context.pos,
//                 token: varName,
//                 params: { var: varName, max: this.maxIndex }
//             });
//             return false;
//         }
//         return true;
//     }
// }

// export class StringVarValidator implements VariableValidator {
//     name = 'string-var';
//     priority = 80;
//     maxIndex = 1;

//     validate(context: ValidationContext, varName: string): boolean {
//         if (!varName.startsWith('$s')) return true;
//         const index = parseInt(varName.substring(2));
//         if (isNaN(index) || index > this.maxIndex || index < 0) {
//             context.errors.push({
//                 key: 'invalidVariableIndex',
//                 line: context.line,
//                 position: context.pos,
//                 token: varName,
//                 params: { var: varName, max: this.maxIndex }
//             });
//             return false;
//         }
//         return true;
//     }
// }

// export class NumberVarValidator implements VariableValidator {
//     name = 'number-var';
//     priority = 70;
//     maxIndex = 9;

//     validate(context: ValidationContext, varName: string): boolean {
//         if (!varName.startsWith('$v')) return true;
//         const index = parseInt(varName.substring(2));
//         if (isNaN(index) || index > this.maxIndex || index < 0) {
//             context.errors.push({
//                 key: 'invalidVariableIndex',
//                 line: context.line,
//                 position: context.pos,
//                 token: varName,
//                 params: { var: varName, max: this.maxIndex }
//             });
//             return false;
//         }
//         return true;
//     }
// }

// export class IntVarValidator implements VariableValidator {
//     name = 'int-var';
//     priority = 60;
//     maxIndex = 9;

//     validate(context: ValidationContext, varName: string): boolean {
//         if (!varName.startsWith('$i')) return true;
//         const index = parseInt(varName.substring(2));
//         if (isNaN(index) || index > this.maxIndex || index < 0) {
//             context.errors.push({
//                 key: 'invalidVariableIndex',
//                 line: context.line,
//                 position: context.pos,
//                 token: varName,
//                 params: { var: varName, max: this.maxIndex }
//             });
//             return false;
//         }
//         return true;
//     }
// }

// export class FloatVarValidator implements VariableValidator {
//     name = 'float-var';
//     priority = 50;
//     maxIndex = 4;

//     validate(context: ValidationContext, varName: string): boolean {
//         if (!varName.startsWith('$f')) return true;
//         const index = parseInt(varName.substring(2));
//         if (isNaN(index) || index > this.maxIndex || index < 0) {
//             context.errors.push({
//                 key: 'invalidVariableIndex',
//                 line: context.line,
//                 position: context.pos,
//                 token: varName,
//                 params: { var: varName, max: this.maxIndex }
//             });
//             return false;
//         }
//         return true;
//     }
// }

// // ============================================================
// // ================ 10. POST-VALIDATION HOOKS =================
// // ============================================================

// export class BlockBalanceHook implements PostValidationHook {
//     name = 'block-balance';
//     priority = 100;

//     execute(context: ValidationContext, result: ValidationResult): ValidationResult {
//         if (context.ifDepth > 0) {
//             result.errors.push({
//                 key: 'missingEnd',
//                 line: context.line,
//                 position: context.pos,
//                 params: { block: 'if' }
//             });
//         }
//         if (context.whileDepth > 0) {
//             result.errors.push({
//                 key: 'missingEnd',
//                 line: context.line,
//                 position: context.pos,
//                 params: { block: 'while' }
//             });
//         }
//         if (context.onDepth > 0) {
//             result.errors.push({
//                 key: 'missingEnd',
//                 line: context.line,
//                 position: context.pos,
//                 params: { block: 'on' }
//             });
//         }
//         result.valid = result.errors.length === 0;
//         return result;
//     }
// }

// // ============================================================
// // ================ 11. EXPRESSION HANDLERS ===================
// // ============================================================

// export class TimeExpressionHandler implements ExpressionHandler {
//     name = 'time';
//     priority = 100;

//     canHandle(engine: ValidatorEngine): boolean {
//         const savePos = engine.getPos();
//         const saveLine = engine.getLine();
//         const saveColumn = engine.getColumn();
        
//         let result = false;
//         try {
//             // const value = engine.parseInt();
//             let suffix = '';
//             while (engine.isLetter(engine.peek())) {
//                 suffix += engine.next();
//             }
//             result = suffix === 's' || suffix === 'ms' || suffix === 'us';
//         } catch (e) {
//             result = false;
//         }
        
//         // Восстанавливаем позицию
//         engine['context'].pos = savePos;
//         engine['context'].line = saveLine;
//         engine['context'].column = saveColumn;
        
//         return result;
//     }

//     parse(engine: ValidatorEngine): any {
//         return engine.parseNumberOrTime();
//     }
// }

// // ============================================================
// // ================ 12. MAIN API ==============================
// // ============================================================

// export class ScriptValidatorAPI {
//     private static engine: ValidatorEngine | null = null;
    
//     static getEngine(): ValidatorEngine {
//         if (!this.engine) {
//             this.engine = new ValidatorEngine();
//         }
//         return this.engine;
//     }
    
//     static validate(script: string): ValidationResult {
//         return this.getEngine().validate(script);
//     }
    
//     static isValid(script: string): boolean {
//         return this.validate(script).valid;
//     }
    
//     static registerCommand(config: CommandConfig): void {
//         this.getEngine().registerCustomCommand(config);
//     }
    
//     static registerVariableValidator(validator: VariableValidator): void {
//         this.getEngine().registerVariableValidator(validator);
//     }
    
//     static registerPostHook(hook: PostValidationHook): void {
//         this.getEngine().registerPostHook(hook);
//     }
    
//     static registerExpressionHandler(handler: ExpressionHandler): void {
//         this.getEngine().registerExpressionHandler(handler);
//     }
// }

// // ============================================================
// // ==================== КОНЕЦ ФАЙЛА ===========================
// // ============================================================