#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @file main.cpp
 * @brief Console chess project in one source file.
 */

/**
 * @brief Цвет шахматной фигуры или пустой клетки.
 */
enum class Color { None, White, Black };

/**
 * @brief Тип шахматной фигуры.
 */
enum class PieceType { Empty, Pawn, Knight, Bishop, Rook, Queen, King };

/**
 * @brief Информация о фигуре на доске.
 */
struct Piece {
    PieceType type = PieceType::Empty; ///< Piece type.
    Color color = Color::None;         ///< Piece color.
};

/**
 * @brief Информация о шахматном ходе.
 */
struct Move {
    int fromRow = 0;                 ///< Start row from 0 to 7.
    int fromCol = 0;                 ///< Start column from 0 to 7.
    int toRow = 0;                   ///< Target row from 0 to 7.
    int toCol = 0;                   ///< Target column from 0 to 7.
    PieceType promotion = PieceType::Queen; ///< Promotion piece for pawn moves.
    bool castle = false;             ///< True if this move is castling.
    bool enPassant = false;          ///< True if this move is en passant capture.
};

/**
 * @brief Возвращает противоположный цвет.
 * @param color Исходный цвет.
 * @return Противоположный цвет.
 */
Color opposite(Color color) {
    if (color == Color::White) return Color::Black;
    if (color == Color::Black) return Color::White;
    return Color::None;
}

/**
 * @brief Проверяет корректность координат доски.
 * @param row Board row.
 * @param col Board column.
 * @return True if coordinates are inside the board.
 */
bool inside(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

/**
 * @brief Преобразует координату вида e2 в индексы доски.
 * @param text Algebraic square coordinate.
 * @return Pair row-column.
 * @throws std::invalid_argument If coordinate is incorrect.
 */
std::pair<int, int> parseSquare(const std::string& text) {
    if (text.size() != 2) {
        throw std::invalid_argument("Square must contain 2 symbols, for example e2");
    }
    char file = static_cast<char>(std::tolower(static_cast<unsigned char>(text[0])));
    char rank = text[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        throw std::invalid_argument("Square must be from a1 to h8");
    }
    return {8 - (rank - '0'), file - 'a'};
}

/**
 * @brief Преобразует фигуру в символ для вывода.
 * @param piece Piece value.
 * @return Character representation.
 */
char pieceChar(const Piece& piece) {
    char c = '.';
    switch (piece.type) {
        case PieceType::Pawn: c = 'p'; break;
        case PieceType::Knight: c = 'n'; break;
        case PieceType::Bishop: c = 'b'; break;
        case PieceType::Rook: c = 'r'; break;
        case PieceType::Queen: c = 'q'; break;
        case PieceType::King: c = 'k'; break;
        case PieceType::Empty: c = '.'; break;
    }
    if (piece.color == Color::White) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return c;
}

/**
 * @brief Основной класс шахматной игры.
 */
class ChessGame {
public:
    /**
     * @brief Creates a new chess game with the standard initial position.
     */
    ChessGame() {
        reset();
    }

    /**
     * @brief Resets the game to the starting chess position.
     */
    void reset() {
        for (auto& row : board_) {
            for (auto& square : row) square = {};
        }
        setupBackRank(0, Color::Black);
        for (int c = 0; c < 8; ++c) board_[1][c] = {PieceType::Pawn, Color::Black};
        for (int c = 0; c < 8; ++c) board_[6][c] = {PieceType::Pawn, Color::White};
        setupBackRank(7, Color::White);
        turn_ = Color::White;
        whiteKingMoved_ = blackKingMoved_ = false;
        whiteARookMoved_ = whiteHRookMoved_ = false;
        blackARookMoved_ = blackHRookMoved_ = false;
        enPassantRow_ = -1;
        enPassantCol_ = -1;
        history_.clear();
    }

    /**
     * @brief Returns current side to move.
     * @return White or Black.
     */
    Color turn() const {
        return turn_;
    }

    /**
     * @brief Returns piece at square.
     * @param row Row from 0 to 7.
     * @param col Column from 0 to 7.
     * @return Piece on the square.
     * @throws std::out_of_range If coordinates are invalid.
     */
    Piece pieceAt(int row, int col) const {
        if (!inside(row, col)) throw std::out_of_range("Board coordinates are outside the board");
        return board_[row][col];
    }

    /**
     * @brief Makes a legal move in coordinate notation.
     * @param from Source square, for example e2.
     * @param to Target square, for example e4.
     * @param promotion Promotion character: q, r, b, or n.
     * @throws std::runtime_error If the move is illegal.
     */
    void makeMoveText(const std::string& from, const std::string& to, char promotion = 'q') {
        auto [fr, fc] = parseSquare(from);
        auto [tr, tc] = parseSquare(to);
        PieceType promo = promotionFromChar(promotion);
        Move wanted{fr, fc, tr, tc, promo, false, false};
        const auto legal = legalMoves(turn_);
        for (const Move& m : legal) {
            if (m.fromRow == wanted.fromRow && m.fromCol == wanted.fromCol &&
                m.toRow == wanted.toRow && m.toCol == wanted.toCol) {
                Move selected = m;
                selected.promotion = promo;
                applyMove(selected);
                history_.push_back(from + "-" + to);
                turn_ = opposite(turn_);
                return;
            }
        }
        throw std::runtime_error("Illegal move");
    }

    /**
     * @brief Checks whether a player is currently in check.
     * @param color Player color.
     * @return True if king is attacked.
     */
    bool inCheck(Color color) const {
        auto king = findKing(color);
        return isSquareAttacked(king.first, king.second, opposite(color));
    }

    /**
     * @brief Checks whether a player is checkmated.
     * @param color Player color.
     * @return True if player is in check and has no legal moves.
     */
    bool isCheckmate(Color color) const {
        return inCheck(color) && legalMoves(color).empty();
    }

    /**
     * @brief Checks whether a player is stalemated.
     * @param color Player color.
     * @return True if player is not in check and has no legal moves.
     */
    bool isStalemate(Color color) const {
        return !inCheck(color) && legalMoves(color).empty();
    }

    /**
     * @brief Returns all legal moves for a player.
     * @param color Player color.
     * @return Vector with legal moves.
     */
    std::vector<Move> legalMoves(Color color) const {
        std::vector<Move> result;
        for (const Move& move : pseudoMoves(color)) {
            ChessGame copy = *this;
            copy.applyMoveNoTurn(move);
            if (!copy.inCheck(color)) result.push_back(move);
        }
        return result;
    }

    /**
     * @brief Prints current board to output stream.
     * @param out Output stream.
     */
    void print(std::ostream& out) const {
        out << "\n  a b c d e f g h\n";
        for (int r = 0; r < 8; ++r) {
            out << 8 - r << ' ';
            for (int c = 0; c < 8; ++c) out << pieceChar(board_[r][c]) << ' ';
            out << 8 - r << '\n';
        }
        out << "  a b c d e f g h\n";
        out << (turn_ == Color::White ? "White" : "Black") << " to move\n";
    }

    /**
     * @brief Loads a simple board from 8 strings for tests and debugging.
     * @param rows Rows with characters PNBRQK for white, pnbrqk for black, dot for empty.
     * @param turn Current side to move.
     * @throws std::invalid_argument If board description is incorrect.
     */
    void loadBoard(const std::vector<std::string>& rows, Color turn) {
        if (rows.size() != 8) throw std::invalid_argument("Board must have 8 rows");
        for (int r = 0; r < 8; ++r) {
            if (rows[r].size() != 8) throw std::invalid_argument("Every board row must have 8 columns");
            for (int c = 0; c < 8; ++c) board_[r][c] = pieceFromChar(rows[r][c]);
        }
        turn_ = turn;
        whiteKingMoved_ = blackKingMoved_ = true;
        whiteARookMoved_ = whiteHRookMoved_ = true;
        blackARookMoved_ = blackHRookMoved_ = true;
        enPassantRow_ = enPassantCol_ = -1;
        history_.clear();
    }

private:
    std::array<std::array<Piece, 8>, 8> board_{};
    Color turn_ = Color::White;
    bool whiteKingMoved_ = false;
    bool blackKingMoved_ = false;
    bool whiteARookMoved_ = false;
    bool whiteHRookMoved_ = false;
    bool blackARookMoved_ = false;
    bool blackHRookMoved_ = false;
    int enPassantRow_ = -1;
    int enPassantCol_ = -1;
    std::vector<std::string> history_;

    void setupBackRank(int row, Color color) {
        board_[row][0] = {PieceType::Rook, color};
        board_[row][1] = {PieceType::Knight, color};
        board_[row][2] = {PieceType::Bishop, color};
        board_[row][3] = {PieceType::Queen, color};
        board_[row][4] = {PieceType::King, color};
        board_[row][5] = {PieceType::Bishop, color};
        board_[row][6] = {PieceType::Knight, color};
        board_[row][7] = {PieceType::Rook, color};
    }

    static PieceType promotionFromChar(char ch) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (ch == 'q') return PieceType::Queen;
        if (ch == 'r') return PieceType::Rook;
        if (ch == 'b') return PieceType::Bishop;
        if (ch == 'n') return PieceType::Knight;
        throw std::invalid_argument("Promotion must be q, r, b or n");
    }

    static Piece pieceFromChar(char ch) {
        if (ch == '.') return {};
        Color color = std::isupper(static_cast<unsigned char>(ch)) ? Color::White : Color::Black;
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (ch == 'p') return {PieceType::Pawn, color};
        if (ch == 'n') return {PieceType::Knight, color};
        if (ch == 'b') return {PieceType::Bishop, color};
        if (ch == 'r') return {PieceType::Rook, color};
        if (ch == 'q') return {PieceType::Queen, color};
        if (ch == 'k') return {PieceType::King, color};
        throw std::invalid_argument("Unknown board character");
    }

    std::pair<int, int> findKing(Color color) const {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (board_[r][c].type == PieceType::King && board_[r][c].color == color) return {r, c};
            }
        }
        throw std::runtime_error("King not found");
    }

    bool isSquareAttacked(int row, int col, Color attacker) const {
        const int pawnDir = attacker == Color::White ? -1 : 1;
        for (int dc : {-1, 1}) {
            int r = row - pawnDir;
            int c = col + dc;
            if (inside(r, c) && board_[r][c].type == PieceType::Pawn && board_[r][c].color == attacker) return true;
        }

        const std::array<std::pair<int, int>, 8> knightDirs{{{1, 2}, {2, 1}, {-1, 2}, {-2, 1}, {1, -2}, {2, -1}, {-1, -2}, {-2, -1}}};
        for (auto [dr, dc] : knightDirs) {
            int r = row + dr;
            int c = col + dc;
            if (inside(r, c) && board_[r][c].type == PieceType::Knight && board_[r][c].color == attacker) return true;
        }

        const std::array<std::pair<int, int>, 4> bishopDirs{{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};
        if (rayAttacked(row, col, attacker, bishopDirs, PieceType::Bishop, PieceType::Queen)) return true;
        const std::array<std::pair<int, int>, 4> rookDirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        if (rayAttacked(row, col, attacker, rookDirs, PieceType::Rook, PieceType::Queen)) return true;

        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int r = row + dr;
                int c = col + dc;
                if (inside(r, c) && board_[r][c].type == PieceType::King && board_[r][c].color == attacker) return true;
            }
        }
        return false;
    }

    template <std::size_t N>
    bool rayAttacked(int row, int col, Color attacker, const std::array<std::pair<int, int>, N>& dirs,
                     PieceType first, PieceType second) const {
        for (auto [dr, dc] : dirs) {
            int r = row + dr;
            int c = col + dc;
            while (inside(r, c)) {
                const Piece p = board_[r][c];
                if (p.type != PieceType::Empty) {
                    if (p.color == attacker && (p.type == first || p.type == second)) return true;
                    break;
                }
                r += dr;
                c += dc;
            }
        }
        return false;
    }

    std::vector<Move> pseudoMoves(Color color) const {
        std::vector<Move> moves;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (board_[r][c].color != color) continue;
                switch (board_[r][c].type) {
                    case PieceType::Pawn: addPawnMoves(moves, r, c, color); break;
                    case PieceType::Knight: addKnightMoves(moves, r, c, color); break;
                    case PieceType::Bishop: {
                        const std::array<std::pair<int, int>, 4> dirs{{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};
                        addSlidingMoves(moves, r, c, color, dirs);
                        break;
                    }
                    case PieceType::Rook: {
                        const std::array<std::pair<int, int>, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
                        addSlidingMoves(moves, r, c, color, dirs);
                        break;
                    }
                    case PieceType::Queen: {
                        const std::array<std::pair<int, int>, 8> dirs{{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
                        addSlidingMoves(moves, r, c, color, dirs);
                        break;
                    }
                    case PieceType::King: addKingMoves(moves, r, c, color); break;
                    case PieceType::Empty: break;
                }
            }
        }
        return moves;
    }

    void addMoveIfValid(std::vector<Move>& moves, int fr, int fc, int tr, int tc, Color color) const {
        if (!inside(tr, tc)) return;
        if (board_[tr][tc].color != color) moves.push_back({fr, fc, tr, tc});
    }

    void addPawnMoves(std::vector<Move>& moves, int r, int c, Color color) const {
        int dir = color == Color::White ? -1 : 1;
        int startRow = color == Color::White ? 6 : 1;
        int promoteRow = color == Color::White ? 0 : 7;
        int one = r + dir;
        if (inside(one, c) && board_[one][c].type == PieceType::Empty) {
            addPawnTarget(moves, r, c, one, c, promoteRow, false);
            int two = r + 2 * dir;
            if (r == startRow && board_[two][c].type == PieceType::Empty) moves.push_back({r, c, two, c});
        }
        for (int dc : {-1, 1}) {
            int tr = r + dir;
            int tc = c + dc;
            if (!inside(tr, tc)) continue;
            if (board_[tr][tc].type != PieceType::Empty && board_[tr][tc].color == opposite(color)) {
                addPawnTarget(moves, r, c, tr, tc, promoteRow, false);
            }
            if (tr == enPassantRow_ && tc == enPassantCol_) {
                Move m{r, c, tr, tc};
                m.enPassant = true;
                moves.push_back(m);
            }
        }
    }

    void addPawnTarget(std::vector<Move>& moves, int fr, int fc, int tr, int tc, int promoteRow, bool enPassant) const {
        if (tr == promoteRow) {
            for (PieceType promo : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
                moves.push_back({fr, fc, tr, tc, promo, false, enPassant});
            }
        } else {
            moves.push_back({fr, fc, tr, tc, PieceType::Queen, false, enPassant});
        }
    }

    void addKnightMoves(std::vector<Move>& moves, int r, int c, Color color) const {
        const std::array<std::pair<int, int>, 8> dirs{{{1, 2}, {2, 1}, {-1, 2}, {-2, 1}, {1, -2}, {2, -1}, {-1, -2}, {-2, -1}}};
        for (auto [dr, dc] : dirs) addMoveIfValid(moves, r, c, r + dr, c + dc, color);
    }

    template <std::size_t N>
    void addSlidingMoves(std::vector<Move>& moves, int r, int c, Color color, const std::array<std::pair<int, int>, N>& dirs) const {
        for (auto [dr, dc] : dirs) {
            int tr = r + dr;
            int tc = c + dc;
            while (inside(tr, tc)) {
                if (board_[tr][tc].color == color) break;
                moves.push_back({r, c, tr, tc});
                if (board_[tr][tc].color == opposite(color)) break;
                tr += dr;
                tc += dc;
            }
        }
    }

    void addKingMoves(std::vector<Move>& moves, int r, int c, Color color) const {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr != 0 || dc != 0) addMoveIfValid(moves, r, c, r + dr, c + dc, color);
            }
        }
        addCastlingMoves(moves, color);
    }

    void addCastlingMoves(std::vector<Move>& moves, Color color) const {
        int row = color == Color::White ? 7 : 0;
        bool kingMoved = color == Color::White ? whiteKingMoved_ : blackKingMoved_;
        bool aRookMoved = color == Color::White ? whiteARookMoved_ : blackARookMoved_;
        bool hRookMoved = color == Color::White ? whiteHRookMoved_ : blackHRookMoved_;
        if (kingMoved || inCheck(color)) return;
        Color enemy = opposite(color);
        if (!hRookMoved && board_[row][5].type == PieceType::Empty && board_[row][6].type == PieceType::Empty &&
            board_[row][7].type == PieceType::Rook && board_[row][7].color == color &&
            !isSquareAttacked(row, 5, enemy) && !isSquareAttacked(row, 6, enemy)) {
            Move m{row, 4, row, 6};
            m.castle = true;
            moves.push_back(m);
        }
        if (!aRookMoved && board_[row][1].type == PieceType::Empty && board_[row][2].type == PieceType::Empty && board_[row][3].type == PieceType::Empty &&
            board_[row][0].type == PieceType::Rook && board_[row][0].color == color &&
            !isSquareAttacked(row, 3, enemy) && !isSquareAttacked(row, 2, enemy)) {
            Move m{row, 4, row, 2};
            m.castle = true;
            moves.push_back(m);
        }
    }

    void applyMove(const Move& move) {
        applyMoveNoTurn(move);
    }

    void applyMoveNoTurn(const Move& move) {
        Piece moving = board_[move.fromRow][move.fromCol];
        Piece captured = board_[move.toRow][move.toCol];
        updateCastlingFlags(move, moving, captured);

        board_[move.fromRow][move.fromCol] = {};
        if (move.enPassant) {
            int capturedRow = moving.color == Color::White ? move.toRow + 1 : move.toRow - 1;
            board_[capturedRow][move.toCol] = {};
        }
        if (move.castle) {
            if (move.toCol == 6) {
                board_[move.toRow][5] = board_[move.toRow][7];
                board_[move.toRow][7] = {};
            } else if (move.toCol == 2) {
                board_[move.toRow][3] = board_[move.toRow][0];
                board_[move.toRow][0] = {};
            }
        }

        if (moving.type == PieceType::Pawn && (move.toRow == 0 || move.toRow == 7)) {
            moving.type = move.promotion;
        }
        board_[move.toRow][move.toCol] = moving;

        enPassantRow_ = -1;
        enPassantCol_ = -1;
        if (moving.type == PieceType::Pawn && std::abs(move.toRow - move.fromRow) == 2) {
            enPassantRow_ = (move.fromRow + move.toRow) / 2;
            enPassantCol_ = move.fromCol;
        }
    }

    void updateCastlingFlags(const Move& move, Piece moving, Piece captured) {
        if (moving.type == PieceType::King) {
            if (moving.color == Color::White) whiteKingMoved_ = true;
            if (moving.color == Color::Black) blackKingMoved_ = true;
        }
        if (moving.type == PieceType::Rook) markRookMoved(move.fromRow, move.fromCol);
        if (captured.type == PieceType::Rook) markRookMoved(move.toRow, move.toCol);
    }

    void markRookMoved(int row, int col) {
        if (row == 7 && col == 0) whiteARookMoved_ = true;
        if (row == 7 && col == 7) whiteHRookMoved_ = true;
        if (row == 0 && col == 0) blackARookMoved_ = true;
        if (row == 0 && col == 7) blackHRookMoved_ = true;
    }
};

#ifndef CHESS_TESTING
/**
 * @brief Program entry point.
 * @return Exit code.
 */
int main() {
    ChessGame game;
    std::cout << "Console Chess C++\n";
    std::cout << "Enter moves as: e2 e4 or e7 e8 q. Enter exit to quit.\n";

    while (true) {
        try {
            game.print(std::cout);
            if (game.isCheckmate(game.turn())) {
                std::cout << "Checkmate. " << (game.turn() == Color::White ? "Black" : "White") << " wins.\n";
                break;
            }
            if (game.isStalemate(game.turn())) {
                std::cout << "Stalemate. Draw.\n";
                break;
            }
            if (game.inCheck(game.turn())) std::cout << "Check!\n";
            std::cout << "> ";
            std::string line;
            if (!std::getline(std::cin, line)) break;
            if (line == "exit" || line == "quit") break;
            std::istringstream input(line);
            std::string from;
            std::string to;
            char promotion = 'q';
            input >> from >> to >> promotion;
            if (from.empty() || to.empty()) {
                std::cout << "Enter move in format: e2 e4\n";
                continue;
            }
            game.makeMoveText(from, to, promotion);
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << "\n";
        }
    }
    return 0;
}
#endif
