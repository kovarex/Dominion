#include <stdint.h>
#include <vector>
#include <deque>
#include <random>
#include <algorithm>
#include <stdarg.h>
#include <varargs.h>
#include <stdio.h>
#include <iomanip>
#include <functional>
#include <cctype>
#include <locale>
#include <codecvt>
#include <windows.h>
#include <tchar.h>
#include <stdarg.h>
#include <sstream>
#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <type_traits>
#include <set>

std::string string_vsprintf(const char* format, va_list args)
{
  va_list tmp_args; //unfortunately you cannot consume a va_list twice
  va_copy(tmp_args, args); //so we have to copy it
  const int required_len = vsnprintf(nullptr, 0, format, tmp_args);
  va_end(tmp_args);

  std::string buf;
  buf.resize(required_len + 1);
  if (std::vsnprintf(&buf[0], buf.size(), format, args) < 0)
    throw std::runtime_error("string_vsprintf encoding error");
  buf.resize(required_len);
  return buf;
}

std::string ssprintf(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  std::string str(string_vsprintf(format, args));
  va_end(args);
  return str;
}

class Log
{
public:
  enum class LogLevel
  {
    Detailed,
    Hands,
    Complete,
  };
  Log(const std::string& name) : name(name) {}
  ~Log()
  {
    FILE* f = fopen(name.c_str(), "w");
    fwrite(content.c_str(), content.size(), 1, f);
    fclose(f);
  }
  void log(LogLevel level, const char* format, ...)
  {
    if (level > this->level)
      return;
    va_list args;
    va_start(args, format);
    this->content += string_vsprintf(format, args);
    va_end(args);
    this->content += "\n";
  }
  void logNoBr(LogLevel level, const char* format, ...)
  {
    if (level > this->level)
      return;
    va_list args;
    va_start(args, format);
    this->content += string_vsprintf(format, args);
    va_end(args);
  }
  std::string content;
  std::string name;
  Log::LogLevel level = LogLevel::Detailed;
};

std::random_device randomDevice;
std::mt19937 randomGenerator(randomDevice());

class Card
{
public:
  enum class Type
  {
    Nothing,
    Copper,
    Silver,
    Gold,
    Estate,
    Douchy,
    Province,
    Smithy
  };

  Card(Type type,
       const char* name,
       uint8_t cost,
       uint8_t tresureValue,
       uint8_t victoryPoints = 0,
       bool isActionCard = false,
       uint8_t drawCards = 0)
    : type(type)
    , name(name)
    , cost(cost)
    , tresureValue(tresureValue)
    , victoryPoints(victoryPoints)
    , isActionCard(isActionCard)
    , drawCards(drawCards) {} 
  Type type;
  const char* name;
  uint8_t cost;
  uint8_t tresureValue;
  uint8_t victoryPoints;
  bool isActionCard;
  uint8_t drawCards;
};

std::vector<Card> cards =
 {
    Card(Card::Type::Nothing, "nothing", 0, 0),
    Card(Card::Type::Copper, "Copper", 0, 1),
    Card(Card::Type::Silver, "Silver", 3, 2),
    Card(Card::Type::Gold, "Gold", 6, 3),
    Card(Card::Type::Estate, "Estate", 2, 0, 1),
    Card(Card::Type::Douchy, "Douchy", 5, 0, 3),
    Card(Card::Type::Province, "Province", 8, 0, 6),
    Card(Card::Type::Smithy, "Smithy", 4, 0, 0, true, 3)
};

const Card& getCard(Card::Type cardType)
{
  return cards[uint32_t(cardType)];
}

class PlayerState
{
public:
  PlayerState()
  { 
    this->reset();
  }
  void reset()
  {
    this->inPlay.clear();
    this->discardDeck.clear();
    this->hand.clear();
    this->drawDeck.clear();
    this->money = 0;
    this->actions = 0;
    this->buys = 0;
    for (uint32_t i = 0; i < 7; ++i)
      discardDeck.push_back(Card::Type::Copper);
    for (uint32_t i = 0; i < 3; ++i)
      discardDeck.push_back(Card::Type::Estate);
    this->drawHand();
    this->turnsPlayed = 0;
  }
  void startTurn()
  {
    this->actions = 1;
    this->buys = 1;
  }

  void endTurn()
  {
    this->money = 0;
    this->actions = 0;
    this->buys = 0;
    this->discardHand();
    this->discardInPlay();
    this->drawHand();
    this->turnsPlayed++;
  }
  void discardHand()
  {
    while (!this->hand.empty())
    {
      Card::Type card = this->hand.back();
      this->hand.pop_back();
      this->discardDeck.push_back(card);
    }
  }
  void discardInPlay()
  {
    while (!this->inPlay.empty())
    {
      Card::Type card = this->inPlay.back();
      this->inPlay.pop_back();
      this->discardDeck.push_back(card);
    }
  }
  void drawHand()
  {
    for (uint32_t i = 0; i < 5; ++i)
      this->drawCard();
  }
  void drawCard()
  {
    if (this->drawDeck.empty())
      this->moveDiscardPileToDrawPile();
    if (!this->drawDeck.empty())
    {
      Card::Type card = this->drawDeck.front();
      this->drawDeck.pop_front();
      this->hand.push_back(card);
    }
  }
  void moveDiscardPileToDrawPile()
  {
    if (this->drawDeck.empty())
      std::swap(this->drawDeck, this->discardDeck);
    else
    {
      while (!this->discardDeck.empty())
      {
        Card::Type card = this->discardDeck.front();
        this->drawDeck.push_back(card);
        this->discardDeck.pop_front();
      }
    }
    std::shuffle(this->drawDeck.begin(), this->drawDeck.end(), randomGenerator);
  }

  void logState(Log& log)
  { 
    log.logNoBr(Log::LogLevel::Hands, "  Hand:");
    for (Card::Type cardType: this->hand)
      log.logNoBr(Log::LogLevel::Hands, " %s", getCard(cardType).name);
    log.logNoBr(Log::LogLevel::Hands, "\n");
    log.logNoBr(Log::LogLevel::Complete,"  Draw:");
    for (Card::Type cardType : this->drawDeck)
      log.logNoBr(Log::LogLevel::Complete, " %s", getCard(cardType).name);
    log.logNoBr(Log::LogLevel::Complete, "\n  Discard:");
    for (Card::Type cardType : this->discardDeck)
      log.logNoBr(Log::LogLevel::Complete, " %s", getCard(cardType).name);
    log.logNoBr(Log::LogLevel::Complete, "\n");
  }

  void moveFromHandToPlay(uint32_t handIndex)
  {
    Card::Type cardType = this->hand[handIndex];
    this->hand.erase(this->hand.begin() + handIndex);
    this->inPlay.push_back(cardType);
  }
  uint32_t getVictoryPoints() const
  {
    uint32_t result = 0;
    for (Card::Type cardType: drawDeck)
      result += getCard(cardType).victoryPoints;
    for (Card::Type cardType : discardDeck)
      result += getCard(cardType).victoryPoints;
    for (Card::Type cardType : hand)
      result += getCard(cardType).victoryPoints;
    for (Card::Type cardType : inPlay)
      result += getCard(cardType).victoryPoints;
    return result;
  }

  uint32_t cardCount(Card::Type cardToCount) const
  {
    uint32_t result = 0;
    for (Card::Type cardType : drawDeck)
      result += (cardToCount == cardType);
    for (Card::Type cardType : discardDeck)
      result += (cardToCount == cardType);
    for (Card::Type cardType : hand)
      result += (cardToCount == cardType);
    for (Card::Type cardType : inPlay)
      result += (cardToCount == cardType);
    return result;
  }

  std::deque<Card::Type> drawDeck;
  std::deque<Card::Type> discardDeck;
  std::vector<Card::Type> hand;
  std::vector<Card::Type> inPlay;
  uint16_t money = 0;
  uint16_t actions = 0;
  uint16_t buys = 0;
  uint32_t turnsPlayed = 0;
};

class GameState;

class Strategy
{
public:
  virtual const char* getName() = 0;
  static constexpr uint32_t NO_CARD = uint32_t(-1);
  virtual uint32_t playAction(const GameState& gameState, const PlayerState& playerState)
  {
    for (uint32_t i = 0; i < playerState.hand.size(); ++i)
      if (getCard(playerState.hand[i]).isActionCard)
        return i;
    return NO_CARD;
  }
  virtual uint32_t playCoin(const GameState& gameState, const PlayerState& playerState)
  {
    for (uint32_t i = 0; i < playerState.hand.size(); ++i)
      if (getCard(playerState.hand[i]).tresureValue != 0)
        return i;
    return NO_CARD;
  }
  virtual Card::Type buy(const GameState& gameState, const PlayerState& playerState)
  {
    return Card::Type::Nothing;
  }
};

class Player
{
public:
  Player() {}
  Player(Strategy* strategy, const std::string& name) : strategy(strategy), name(name) {}
  Player(const Player& player) = delete;
  Player(Player&& player) { this->name = player.name; delete this->strategy; this->strategy = player.strategy; player.strategy = nullptr; }
  ~Player() { delete strategy; }

  PlayerState state;
  Strategy* strategy = nullptr;
  std::string name;
  uint32_t wins = 0;
};

class SupplyPile
{
public:
  Card::Type cardType;
  uint16_t cardsLeft;
};

class GameState
{
public:
  GameState() { this->setupBasicPiles(); }

  std::vector<Player> players;
  std::deque<Card::Type> trashDeck;
  std::vector<SupplyPile> supplyPiles;
  Log* log = nullptr;

  void setupBasicPiles()
  {
    this->supplyPiles.clear();
    this->supplyPiles.push_back(SupplyPile{ Card::Type::Copper, 50 });
    this->supplyPiles.push_back(SupplyPile{ Card::Type::Silver, 50 });
    this->supplyPiles.push_back(SupplyPile{ Card::Type::Gold, 50 });
    this->supplyPiles.push_back(SupplyPile{ Card::Type::Estate, 8 });
    this->supplyPiles.push_back(SupplyPile{ Card::Type::Douchy, 8 });
    this->supplyPiles.push_back(SupplyPile{ Card::Type::Province, 8 });
    this->supplyPiles.push_back(SupplyPile{ Card::Type::Smithy, 10 });
  }

  SupplyPile* getSupply(Card::Type cardType)
  {
    for (SupplyPile& pile: this->supplyPiles)
      if (pile.cardType == cardType)
        return &pile;
    return nullptr;
  }

  const SupplyPile* getSupply(Card::Type cardType) const
  {
    for (const SupplyPile& pile : this->supplyPiles)
      if (pile.cardType == cardType)
        return &pile;
    return nullptr;
  }

  bool canBuy(Card::Type cardType) const
  {
    if (const SupplyPile* supplyPile = getSupply(cardType))
      return supplyPile->cardsLeft > 0;
    return false;
  }

  bool gameEnded() const
  {
    if (this->getSupply(Card::Type::Province)->cardsLeft == 0)
      return true;
    uint32_t emptyPiles = 0;
    for (const SupplyPile& pile: this->supplyPiles)
      if (pile.cardsLeft == 0)
        ++emptyPiles;
    return emptyPiles >= 3;
  }
};

class GameEngine
{
public:
  GameState gameState;
  uint32_t nextPlayer = 0;
  uint32_t draws = 0;

  uint32_t playGame()
  {
    gameState.log = new Log("a");
    gameState.log->level = Log::LogLevel::Hands;
    this->gameState.setupBasicPiles();
    for (Player& player: this->gameState.players)
      player.state.reset();

    for (uint32_t i = 0; i < 1000; ++i)
      if (this->playPlayerTurn())
      {
        if (gameState.log)
          gameState.log->log(Log::LogLevel::Detailed, "The game was finished in %u turns\n", i + 1);
        gameState.log->name = ssprintf("game_%u", i + 1);
        Player* best = nullptr;
        uint32_t bestPoints = 0;
        uint32_t bestTurns = uint32_t(-1);
        for (Player& player: gameState.players)
        {
          uint32_t points = player.state.getVictoryPoints();
          if (gameState.log)
            gameState.log->log(Log::LogLevel::Detailed, "%s (%s) got %u victory points", player.name.c_str(), player.strategy->getName(), points);
          if (best == nullptr || bestPoints < points || bestPoints == points && bestTurns > player.state.turnsPlayed)
          {
            best = &player;
            bestPoints = points;
            bestTurns = player.state.turnsPlayed;
          }
          else if (best && bestPoints == points && bestTurns == player.state.turnsPlayed)
            best = nullptr;
        }
        if (best)
        {
          best->wins++;
          gameState.log->log(Log::LogLevel::Detailed, "%s is the winner", best->name.c_str());
        }
        else
        {
          gameState.log->log(Log::LogLevel::Detailed, "The result is draw");
          this->draws++;
        }
        delete gameState.log;
        gameState.log = nullptr;
        return i;
      }
    printf("Game was not finished in 1000 turns.\n");
    return 1000;
  }

  bool playPlayerTurn()
  {
    Player& player = this->gameState.players[nextPlayer];
    player.state.startTurn();
    if (this->gameState.log)
    {
      this->gameState.log->log(Log::LogLevel::Detailed, "%s", player.name.c_str());
      player.state.logState(*this->gameState.log);
    }
    this->playActions(player);
    this->playCoins(player);
    this->playBuys(player);
    player.state.endTurn();
    ++this->nextPlayer;
    if (this->nextPlayer >= this->gameState.players.size())
      this->nextPlayer = 0;

    return gameState.gameEnded();
  }

  void playActions(Player& player)
  {
    while (player.state.actions > 0)
    {
      uint32_t actionToPlay = player.strategy->playAction(this->gameState, player.state);
      if (actionToPlay == Strategy::NO_CARD)
        return;
      if (player.state.hand.size() <= actionToPlay)
        throw std::runtime_error("Strategy error, ordered to play card out of bounds");
      Card::Type type = player.state.hand[actionToPlay];
      const Card& card = getCard(type);
      if (!card.isActionCard)
        throw std::runtime_error("Ordered to play non action card");
      player.state.moveFromHandToPlay(actionToPlay);
      if (this->gameState.log)
        this->gameState.log->log(Log::LogLevel::Detailed, "  Played %s", card.name);
      for (uint32_t i = 0; i < card.drawCards; ++i)
        player.state.drawCard();
      player.state.actions--;
      if (this->gameState.log)
        player.state.logState(*this->gameState.log);
    }
  }
  void playCoins(Player& player)
  {
    while (true)
    {
      uint32_t coinToPlay = player.strategy->playCoin(this->gameState, player.state);
      if (coinToPlay == Strategy::NO_CARD)
        return;
      if (player.state.hand.size() <= coinToPlay)
        throw std::runtime_error("Strategy error, ordered to play card out of bounds");
      Card::Type type = player.state.hand[coinToPlay];
      uint8_t tresureValue = getCard(type).tresureValue;
      if (tresureValue == 0)
        throw std::runtime_error("Strategy error, ordered to play non tresure card as treasure card");
      player.state.money += tresureValue;
      player.state.moveFromHandToPlay(coinToPlay);
    }
  }
  void playBuys(Player& player)
  {
    while (player.state.buys > 0)
    {
      Card::Type cardToBuy = player.strategy->buy(this->gameState, player.state);
      if (cardToBuy == Card::Type::Nothing)
        return;
      this->buyCard(player, cardToBuy);
    }
  }
  void buyCard(Player& player, Card::Type cardType)
  {
    for (SupplyPile& supplyPile: this->gameState.supplyPiles)
      if (supplyPile.cardType == cardType)
      {
        if (supplyPile.cardsLeft == 0)
          throw std::runtime_error("Ordered to buy card that has no cards left.");
        const Card& card = getCard(cardType);
        if (card.cost > player.state.money)
          throw std::runtime_error("Ordered to buy card that is too expensive.");
        supplyPile.cardsLeft--;
        player.state.discardDeck.push_back(cardType);
        player.state.money -= card.cost;
        player.state.buys--;
        if (gameState.log)
         gameState.log->log(Log::LogLevel::Detailed, "  Bought %s\n", card.name);
        return;
      }
    throw std::runtime_error("Ordered to buy card that has no supply pile");
  }

};

class JustMoneyStrategy : public Strategy
{
public:
  virtual const char* getName() override { return "Just Money"; }
  virtual Card::Type buy(const GameState& gameState, const PlayerState& playerState)
  {
    if (playerState.money >= 8)
      return Card::Type::Province;
    if (playerState.money >= 6)
      return Card::Type::Gold;
    if (playerState.money >= 3)
      return Card::Type::Silver;
    return Card::Type::Nothing;
  }
};

class JustMoneyOneEstate : public Strategy
{
public:
  virtual const char* getName() override { return "Just Money buy one extra estate"; }
  virtual Card::Type buy(const GameState& gameState, const PlayerState& playerState)
  {
    if (playerState.money >= 8)
      return Card::Type::Province;
    if (playerState.money >= 6)
      return Card::Type::Gold;
    if (playerState.money >= 2 && playerState.cardCount(Card::Type::Estate) == 3 && playerState.cardCount(Card::Type::Gold) > 1)
      return Card::Type::Estate;
    if (playerState.money >= 3)
      return Card::Type::Silver;
    return Card::Type::Nothing;
  }
};

class JustMoneyOneEstateWithSmithy : public Strategy
{
public:
  virtual const char* getName() override { return "Just Money buy one extra estate and 2 smithies"; }
  virtual Card::Type buy(const GameState& gameState, const PlayerState& playerState)
  {
    if (playerState.money >= 8)
      return Card::Type::Province;
    if (playerState.money >= 6)
      return Card::Type::Gold;
    if (playerState.money >= 4 && gameState.canBuy(Card::Type::Smithy) && playerState.cardCount(Card::Type::Smithy) < 2)
      return Card::Type::Smithy;
    if (playerState.money >= 2 && playerState.cardCount(Card::Type::Estate) == 3 && playerState.cardCount(Card::Type::Gold) > 1)
      return Card::Type::Estate;
    if (playerState.money >= 3)
      return Card::Type::Silver;
    return Card::Type::Nothing;
  }
};

class JustMoneyStrategyWithDouchy : public Strategy
{
public:
  virtual const char* getName() override { return "Just Money with douchy"; }
  virtual Card::Type buy(const GameState& gameState, const PlayerState& playerState)
  {
    if (playerState.money >= 8)
      return Card::Type::Province;
    if (playerState.money >= 6)
      return Card::Type::Gold;
    if (playerState.money >= 5 && gameState.canBuy(Card::Type::Douchy))
      return Card::Type::Douchy;
    if (playerState.money >= 3)
      return Card::Type::Silver;
    return Card::Type::Nothing;
  }
};

int main()
{
  GameEngine gameEngine;
  gameEngine.gameState.players.push_back(Player(new JustMoneyOneEstate(), "Player 1"));
  gameEngine.gameState.players.push_back(Player(new JustMoneyOneEstateWithSmithy(), "Player 2"));
  uint32_t turns = 0;
  uint32_t repetitions = 10000;
  for (uint32_t i = 0; i < repetitions; ++i)
    turns += gameEngine.playGame();
  printf("Average turns to finish the game was %g\n", double(turns) / double(repetitions));
  printf("Draws: %u\n", gameEngine.draws);
  printf("Wins:\n");
  for (Player& player: gameEngine.gameState.players)
  {
    printf("  %s (%s): %u\n", player.name.c_str(), player.strategy->getName(), player.wins);
  }
  return 0;
}
